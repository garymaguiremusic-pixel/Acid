#pragma once

#include "PageBase.h"
#include "../BandGraph.h"
#include "../TubeView.h"
#include "../arp/Controls.h"

class TubePage : public PageBase
{
public:
    explicit TubePage (AcidAudioProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void update() override;

    BandGraph graph;

private:
    // the Saturate / Foldback distortion (top)
    juce::TextButton distOnButton { "ON" };
    SegmentedButtons distModeButtons;
    AcidKnob distDrive { AcidKnob::Size::Small, "Drive" }, distAmount { AcidKnob::Size::Small, "Distort" };

    // the three-band tube
    juce::TextButton tubeOnButton { "ON" };
    juce::TextButton generateButton { "GENERATE DISTORTION" }, undoButton { "UNDO" }, initButton { "INIT" }, redoButton { "REDO" };
    AcidKnob resonator { AcidKnob::Size::Small, "Resonator" }, character { AcidKnob::Size::Small, "Character" };
    AcidKnob input { AcidKnob::Size::Small, "Input" }, output { AcidKnob::Size::Small, "Output" }, mix { AcidKnob::Size::Small, "Mix" };

    struct BandControls
    {
        AcidKnob drive { AcidKnob::Size::Large, "Drive" };
        AcidKnob gain { AcidKnob::Size::Small, "Gain" };
        juce::TextButton solo { "Solo" };
        TubeView tube;
        InfoLcd driveLcd;
    };
    BandControls bands[3];
    float lastGraphState[7] {};
};
