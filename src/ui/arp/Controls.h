#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

//==============================================================================
/**
    A row of radio buttons bound to a choice or int parameter. Segment i sets
    the parameter to (minimum + i), so it works for "1 2 3 4" octave counts as
    well as choice lists.
*/
class SegmentedButtons : public juce::Component
{
public:
    SegmentedButtons (juce::RangedAudioParameter&, const juce::StringArray& labels, const juce::String& tooltip);

    void resized() override;
    /** Lays the buttons out in this many rows (default 1). */
    void setRows (int r)    { rows = juce::jmax (1, r); resized(); }

private:
    void select (int index);
    int rows = 1;

    juce::RangedAudioParameter& param;
    juce::OwnedArray<juce::TextButton> buttons;
    juce::ParameterAttachment attachment;
    int minimum = 0;
};

//==============================================================================
/** Blue LCD showing the randomiser seed. Double-click to type a seed; Enter applies it. */
class SeedField : public juce::Label
{
public:
    SeedField();

    void setSeed (juce::uint32);
    std::function<void (juce::uint32)> onSeedEntered;

    void paint (juce::Graphics&) override;

private:
    juce::uint32 seed = 1;
};

//==============================================================================
/** A Virus-style button showing a left or right arrow (sample Back / Next). */
class ArrowIconButton : public juce::Button
{
public:
    explicit ArrowIconButton (bool pointsRight) : juce::Button (pointsRight ? "next" : "back"), right (pointsRight) {}
    void paintButton (juce::Graphics&, bool isHighlighted, bool isDown) override;

private:
    bool right;
};
