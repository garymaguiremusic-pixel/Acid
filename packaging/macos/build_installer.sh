#!/usr/bin/env bash
# Builds the macOS installer (.pkg) for the Acid VST3 + AU plugins.
#
#   packaging/macos/build_installer.sh <artefacts dir> <version> <output dir>
#
# <artefacts dir> is build/Acid_artefacts/Release (it holds VST3/ and AU/).
#
# The plugins are ad-hoc signed (Apple Silicon refuses to load unsigned code) and the
# installer itself is unsigned. Plugins put in place by Installer don't carry the
# "downloaded from the internet" flag, so DAWs load them without the "Apple cannot
# check it for malicious software" message. Only the .pkg needs a right-click > Open
# the first time.
#
# Optional, only with a paid Apple Developer account (then nothing warns at all):
#   MACOS_APP_IDENTITY        "Developer ID Application: Your Name (TEAMID)"
#   MACOS_INSTALLER_IDENTITY  "Developer ID Installer: Your Name (TEAMID)"
#   APPLE_ID, APPLE_TEAM_ID, APPLE_APP_PASSWORD   (notarization)
set -euo pipefail

ARTEFACTS="$1"
VERSION="$2"
OUT="$3"
NAME="Acid"
ID_BASE="com.acidproject.acid"
HERE="$(cd "$(dirname "$0")" && pwd)"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
mkdir -p "$OUT"

VST3="$ARTEFACTS/VST3/$NAME.vst3"
AU="$ARTEFACTS/AU/$NAME.component"
[ -d "$VST3" ] || { echo "missing $VST3"; exit 1; }
[ -d "$AU" ]   || { echo "missing $AU"; exit 1; }

# --- 1. sign and check the plugin bundles --------------------------------------
for bundle in "$VST3" "$AU"; do
    if [ -n "${MACOS_APP_IDENTITY:-}" ]; then
        codesign --force --deep --options runtime --timestamp --sign "$MACOS_APP_IDENTITY" "$bundle"
    else
        codesign --force --deep --sign - "$bundle"      # ad-hoc
    fi
    codesign --verify --deep --strict "$bundle"
    bin="$bundle/Contents/MacOS/$NAME"
    test -x "$bin"
    lipo "$bin" -verify_arch arm64 x86_64
    echo "OK: $(basename "$bundle") ($(lipo -archs "$bin"))"
done

# --- 2. one component package per format --------------------------------------
make_component () {   # <bundle> <install dir> <id suffix>
    local bundle="$1" dest="$2" suffix="$3"
    local root="$WORK/root-$suffix"
    mkdir -p "$root$dest"
    ditto "$bundle" "$root$dest/$(basename "$bundle")"

    # don't let Installer "relocate" the bundle to wherever an older copy lives
    pkgbuild --analyze --root "$root" "$WORK/$suffix.plist"
    local i=0
    while /usr/libexec/PlistBuddy -c "Set :$i:BundleIsRelocatable false" "$WORK/$suffix.plist" 2>/dev/null; do i=$((i+1)); done

    pkgbuild --root "$root" \
             --component-plist "$WORK/$suffix.plist" \
             --identifier "$ID_BASE.$suffix" \
             --version "$VERSION" \
             --install-location / \
             "$WORK/$NAME-$suffix.pkg"
    # the payload must contain the plugin binary
    pkgutil --payload-files "$WORK/$NAME-$suffix.pkg" | grep -E "/MacOS/$NAME\$"
}
make_component "$VST3" "/Library/Audio/Plug-Ins/VST3" vst3
make_component "$AU"   "/Library/Audio/Plug-Ins/Components" au

# --- 3. the product installer (both formats; Customize lets you untick one) ----
sed -e "s/@VERSION@/$VERSION/g" "$HERE/distribution.xml" > "$WORK/distribution.xml"
mkdir -p "$WORK/resources"
cp "$HERE/welcome.html" "$WORK/resources/"

PKG="$OUT/ACID-macOS-Installer.pkg"
SIGN_ARGS=()
if [ -n "${MACOS_INSTALLER_IDENTITY:-}" ]; then
    SIGN_ARGS=(--sign "$MACOS_INSTALLER_IDENTITY" --timestamp)
fi
productbuild --distribution "$WORK/distribution.xml" \
             --resources "$WORK/resources" \
             --package-path "$WORK" \
             ${SIGN_ARGS[@]+"${SIGN_ARGS[@]}"} \
             "$PKG"

# --- 4. notarize and staple (Developer ID only) ---------------------------------
if [ -n "${MACOS_INSTALLER_IDENTITY:-}" ] && [ -n "${APPLE_ID:-}" ] && [ -n "${APPLE_TEAM_ID:-}" ] && [ -n "${APPLE_APP_PASSWORD:-}" ]; then
    xcrun notarytool submit "$PKG" --apple-id "$APPLE_ID" --team-id "$APPLE_TEAM_ID" \
                                   --password "$APPLE_APP_PASSWORD" --wait --timeout 30m
    xcrun stapler staple "$PKG"
    spctl --assess --type install --verbose=4 "$PKG"
fi

echo "Built $PKG"
