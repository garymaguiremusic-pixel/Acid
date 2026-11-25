#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

class AcidAudioProcessor;

/** Three-band display on a log-frequency / dB grid. Drag the edge handles (top) to
    move the crossovers, drag the band centres up and down to set each band's gain. */
class BandGraph : public juce::Component
{
public:
    explicit BandGraph (AcidAudioProcessor&);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

    juce::String getHoverHelp() const { return hoverHelp; }

private:
    juce::Rectangle<float> plotArea() const;
    float xForFreq (float hz) const;
    float freqForX (float x) const;
    float yForDb (float db) const;
    float dbForY (float y) const;
    float edge (int i) const;
    float gain (int band) const;
    int findHandle (juce::Point<float>, bool& isEdge) const;

    AcidAudioProcessor& processor;
    juce::RangedAudioParameter* edgeParams[4] {};
    juce::RangedAudioParameter* gainParams[3] {};
    juce::RangedAudioParameter* dragParam = nullptr;
    int dragIndex = -1;
    bool dragEdge = false;
    juce::String hoverHelp;
};
