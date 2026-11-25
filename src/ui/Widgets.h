#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/** Attaches hover-help text to any component (shown in the header display). */
inline void setHelp (juce::Component& c, const juce::String& text) { c.getProperties().set ("help", text); }

//==============================================================================
/** The header display: bank tag, preset name and the hover help line. Click the
    name to browse presets. */
class HelpDisplay : public juce::Component
{
public:
    void setPreset (const juce::String& bank, const juce::String& name);
    void setHelpText (const juce::String& text);
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override  { if (onClick) onClick(); }

    std::function<void()> onClick;

private:
    juce::String bankText, nameText, helpText;
};

//==============================================================================
/** LCD readout: name on the left, value (orange) on the right. */
class InfoLcd : public juce::Component
{
public:
    void setContent (const juce::String& name, const juce::String& value);
    void paint (juce::Graphics&) override;

private:
    juce::String nameText, valueText;
};

//==============================================================================
/** Oscillator wave drawn from the DSP's own shaper. */
class WaveDisplay : public juce::Component
{
public:
    void setMorph (float m)  { if (m != morph) { morph = m; repaint(); } }
    void paint (juce::Graphics&) override;

private:
    float morph = 0.0f;
};

//==============================================================================
/** LFO shape with the moving phase dot. */
class LfoPreview : public juce::Component
{
public:
    void setState (int shape, float phase, float depth, bool mirror);
    void paint (juce::Graphics&) override;

private:
    int shape = 1;
    float phase = 0.0f, depth = 0.0f;
    bool mirror = false;
};

//==============================================================================
/** Two bars: the filter envelope and the accent-sweep charge. */
class EnvMeter : public juce::Component
{
public:
    void setLevels (float env, float accent);
    void paint (juce::Graphics&) override;

private:
    float envLevel = 0.0f, accentLevel = 0.0f;
};

//==============================================================================
class HelpButton : public juce::Button
{
public:
    HelpButton() : juce::Button ("help") {}
    void paintButton (juce::Graphics&, bool isHighlighted, bool isDown) override;
};

/** Quick reference over the editor; click to close. */
class HelpOverlay : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { setVisible (false); }
};
