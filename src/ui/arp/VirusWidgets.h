#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
/** LCD readout, e.g. "SEQUENCE    FILE NAME" (from ArpSampler). */
class LcdDisplay : public juce::Component
{
public:
    void setContent (const juce::String& name, const juce::String& value);
    void setDimmed (bool shouldDim);
    void paint (juce::Graphics&) override;

private:
    juce::String nameText, valueText;
    bool dimmed = false;
};

//==============================================================================
/** Tempo readout. Shows the host tempo, or the draggable internal BPM (whole steps). */
class TempoField : public juce::Component,
                   public juce::SettableTooltipClient
{
public:
    explicit TempoField (juce::RangedAudioParameter& bpmParam);

    void setTempo (float bpm, bool fromHost);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    juce::RangedAudioParameter& param;
    juce::ParameterAttachment attachment;
    float internalBpm = 120.0f, shownBpm = 120.0f, lastY = 0.0f, dragBpm = 120.0f;
    bool hostTempo = false, dragging = false;
};
