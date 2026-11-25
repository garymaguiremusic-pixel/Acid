# Design decisions

Choices made where the spec left room. In each case the aim was an authentic,
playable acid sound.

## Build and format
- **Windows**: VST3. **macOS**: VST3 + AU, universal (arm64 + x86_64), macOS 10.13+,
  shipped as an installer .pkg (the same approach as OTT): the plugins are ad-hoc
  signed, and because Installer puts them in place they carry no quarantine flag, so
  DAWs load them without the malware warning. The .pkg itself is unsigned (right-click >
  Open once). `packaging/macos/build_installer.sh` can also Developer-ID sign and
  notarize if signing identities and Apple credentials are ever provided. An optional
  Standalone target (`-DACID_BUILD_STANDALONE=ON`) is there for local testing.
- **JUCE 8.0.4**, the same version Virus Delay builds with, fetched by CMake.
- The plugin name lives in one CMake cache variable, `ACID_PLUGIN_NAME` (default
  "Acid"). It flows into the product name, the preset folder and the UI.

## GUI
- **Look and feel**: taken from Virus Delay at its latest restyle (black panels, orange
  as the only accent, light grey labels, white highlights, chrome large knobs, black
  small knobs, LCD readouts, mode-style page tabs, bottom title bar with a "?" help).
  All drawing is procedural. There are no image assets and no third-party branding.
- **Size**: the base layout is 1000 x 720 (not the 1600 x 1176 in the spec), so the
  Virus Delay font sizes and control sizes read the same. The window resizes from
  60 % to 200 % with a fixed aspect ratio, scales the whole panel, and remembers its
  size.
- **Knobs** move in whole steps of 1.0: dragging, and the mouse wheel (one notch = one
  step). Double-click resets to the default. Where a real unit fits whole steps
  (semitones, dB, %, ms, steps, voices, degrees) the parameter uses that unit
  directly. Otherwise it runs 0..100 and is mapped to real units (Hz, ms), which the
  knob, the help display and the host all show.
- **Hover help** goes in the header display (bank / preset name / help line) instead
  of tooltips.
- **Light theme**: a pale-grey variant with the same orange accent. The theme is saved
  with the session.
- The **Steps** page buttons (1-16 ... 49-64) sit in the sequencer's control row. The
  pattern **Length** is part of the pattern data (each pattern has its own length,
  with undo), so the Length knob is not an automatable parameter. Clicking a step
  number in the grid also sets the length.

## Sound
- **Filter**: a ZDF/TPT ladder solved exactly every sample (4x4 tridiagonal system)
  with the feedback loop resolved instantly and the ladder input saturated. The diode
  model uses coupled stages with a 110 Hz high-pass in the feedback path, and its
  feedback stops at 94 % of the self-oscillation point (squelch without a clean
  whistle). The transistor model can self-oscillate. Cutoff is calibrated so the
  resonant peak sits on the knob's frequency. Both run at 2x oversampling, together
  with the oscillators and the drive.
- **Cutoff Low** is the floor the envelope falls back to. When it is at or above
  Cutoff, the filter behaves exactly like the classic circuit (base = Cutoff, the
  envelope adds Env Mod on top).
- **Accent** has its own envelope (Env Attack / Acc Decay) that drives the VCA boost and
  the accent circuit. Acc Sweep sets how slowly the accent "capacitor" charges and
  discharges, so runs of accents build up (covered by a unit test). Accented notes also
  use Acc Decay for the filter envelope.
- **Amp envelope**: Decay at its maximum means *hold*, and that is the default. Slid and
  tied notes then sustain like the hardware; the 170 ms default from the reference
  screenshot cut tied notes short. Attack (0.25 ms) and Release (4.5 ms) use the
  screenshot defaults.
- **LFO Mirror** makes the LFO unipolar (0..1) instead of bipolar. The Depth sign
  already covers inverting.
- **LFO depth ranges**: Cutoff +-4 octaves, Pitch +-12 semitones, Wave +-full morph,
  Reso +-full range, Volume = tremolo down to silence at 100 %.
- **Tube**: each band is divided by the square root of its tube's small-signal gain,
  so zero drive is unity and more drive adds mostly saturation with a moderate rise
  in level. The tube curve is the spec's biased tanh,
  normalised so +-1 in gives about +-1 out. Bypassing keeps the oversampling latency
  (the dry path is delayed to match), so the host's delay compensation never changes.
- **Output**: a gentle safety soft-clip above about -2 dBFS on the master. Factory
  presets peak around -7 to -5 dBFS.
- **Latency** reported to the host: 15 samples (voice oversampling) + 19 samples (tube
  oversampling).

## Sequencer and MIDI
- With **Seq on** the pattern plays whenever the host plays (or the Play button is
  on). While nothing is running, incoming MIDI still plays the synth directly, so the
  keyboard always works. While the pattern runs, incoming notes are ignored (except
  in Key mode).
- **Key mode**: holding a key transposes the pattern by (key - 60) and runs it. With the
  host stopped, the first key restarts the pattern from step 1 at that exact sample.
- **Direct MIDI play**: last-note priority; an overlapping (legato) note slides;
  velocity >= 100 is an accent.
- **Swing** 0..75 %: at 75 % the off-beat steps land on the dotted position.
- **Ratchets** are only played on steps that start a new note (not on slid-into notes).
- **MIDI out** (on by default, toggled in the MIDI menu): slides are sent as
  overlapping notes, accents as velocity 127, normal notes as 100.
- **Factory presets** are built into the plugin. User presets are JSON files in
  `Documents/Acid/Presets` (params + 16 patterns + name + bank tag). Hosts that
  browse "programs" see the factory presets.
- File layout: the spec named `DiodeLadder` / `TransistorLadder` / `AccentCircuit` /
  `Unison` / `UndoStack` separately. They live in `dsp/Ladder` (one class, two
  models), `dsp/Envelopes` (envelopes + accent circuit), `dsp/Voice` (unison is part
  of the voice) and `seq/PatternTools` (undo stack).
