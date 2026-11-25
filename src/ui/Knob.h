#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
/** Rotary knob with the Virus Delay feel:
      - vertical drag and the mouse wheel move in whole steps of 1.0
        (one wheel notch = one step)
      - double-click resets to the default
      - small knobs show their label underneath, which swaps to the value while
        hovering or dragging
    Drawing is done by AcidLookAndFeel. */
class AcidKnob : public juce::Slider
{
public:
    enum class Size { Large, Small };

    AcidKnob (Size, const juce::String& labelText);

    Size getKnobSize() const noexcept           { return knobSize; }
    void setDefaultValue (double v) noexcept    { defaultValue = v; }
    void setBipolar (bool b) noexcept           { bipolar = b; }
    bool isBipolar() const noexcept             { return bipolar; }

    /** Thin modulation arc (as a fraction of the knob range, 0 = none). */
    void setModDepth (float d)                  { if (std::abs (d - modDepth) > 0.002f) { modDepth = d; repaint(); } }
    float getModDepth() const noexcept          { return modDepth; }

    std::function<void()> onGestureBegin, onGestureEnd;
    std::function<juce::String (double)> displayValue;   // value text with units
    juce::String helpText;                                // one-line explanation for the help display

    /** "CUTOFF  447 Hz  -  Filter cutoff ..." for the help display. */
    juce::String getHelpLine();
    const juce::String& getLabel() const noexcept { return label; }

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    void setValueWithGesture (double v);
    juce::String valueText();

    Size knobSize;
    juce::String label;
    double defaultValue = 0.0;
    float lastDragY = 0.0f;
    double dragValue = 0.0;
    bool dragging = false, hovering = false, bipolar = false;
    float modDepth = 0.0f;
};

//==============================================================================
/** Connects an AcidKnob to a parameter with proper host gesture handling. */
class KnobAttachment
{
public:
    KnobAttachment (juce::RangedAudioParameter&, AcidKnob&);
    ~KnobAttachment();

private:
    AcidKnob& knob;
    juce::ParameterAttachment attachment;
};
