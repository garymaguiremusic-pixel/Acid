#pragma once

// Lets the UI code that came from ArpSampler draw with Acid's look-and-feel and theme:
// CustomLookAndFeel is Acid's look-and-feel, and VirusColours holds the current theme's
// colours (refreshed whenever the theme changes).

#include "../LookAndFeel.h"

using CustomLookAndFeel = AcidLookAndFeel;

namespace VirusColours
{
    inline juce::Colour background, headerText, labelText, bevelDark, insetBottom, lcdBottom, accent, accentGlow,
                        accentLight, lightGrey, red, buttonOnTop, buttonOnBottom, buttonOffTop, buttonOffBottom,
                        buttonText, popupHighlight, dimText;

    inline void refresh()
    {
        const auto& t = Theme::get();
        background = t.background2;     headerText = t.headerText;     labelText = t.labelText;
        bevelDark = t.bevelDark;        insetBottom = t.insetBottom;   lcdBottom = t.lcdBottom;
        accent = t.accent;              accentGlow = t.accentGlow;     accentLight = t.accentLight;
        lightGrey = t.lightGrey;        red = t.red;
        buttonOnTop = juce::Colour (0xffff8a2a);  buttonOnBottom = juce::Colour (0xffc4520a);
        buttonOffTop = t.buttonTop;     buttonOffBottom = t.buttonBottom;  buttonText = t.buttonText;
        popupHighlight = juce::Colour (0xffc4520a);
        dimText = juce::Colour (0xff6e6e72);
    }
}
