#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
/** Colour sets. Dark is the Virus Delay palette (black panels, orange accent, light
    grey labels, white highlights); Light is a pale-grey version with the same accent. */
struct Theme
{
    juce::Colour background1, background2, panelTop, panelBottom, headerTop, headerBottom, headerText,
                 labelText, text, bevelDark, bevelLight, insetTop, insetBottom, lcdTop, lcdBottom,
                 lcdDimTop, lcdDimBottom, accent, accentGlow, accentLight, lightGrey, red,
                 buttonTop, buttonBottom, buttonText, barTop, barBottom, tickColour;

    static const Theme& get();
    static void setLight (bool light);
    static bool isLight();
};

//==============================================================================
/** Draws every control procedurally (gradients, paths, shadows): no image assets. */
class AcidLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AcidLookAndFeel();
    void refreshColours();

    static juce::Font uiFont (float height, bool bold = false);

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;

    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getPopupMenuFont() override;
    void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool isHighlighted, bool isDown) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool isHighlighted, bool isDown) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

    // Shared drawing helpers
    static void drawPanel (juce::Graphics&, juce::Rectangle<float> bounds, const juce::String& title);
    static void drawInsetBox (juce::Graphics&, juce::Rectangle<float> bounds, float corner = 3.0f);
    static void drawLcd (juce::Graphics&, juce::Rectangle<float> bounds, bool dimmed = false);
    static void drawSmallLabel (juce::Graphics&, const juce::String& text, juce::Rectangle<float> area,
                                juce::Justification = juce::Justification::centred);
    static void drawLargeKnob (juce::Graphics&, juce::Rectangle<float> area, float pos, float start, float end,
                               float modDepth = 0.0f);
    static void drawSmallKnob (juce::Graphics&, juce::Rectangle<float> area, float pos, float start, float end,
                               float modDepth = 0.0f, bool bipolar = false);

private:
    static void drawTicks (juce::Graphics&, juce::Point<float> centre, float rInner, float rOuter,
                           float start, float end, int count, juce::Colour, float thickness);
    static void drawModArc (juce::Graphics&, juce::Point<float> c, float r, float pos, float start, float end, float depth);
};
