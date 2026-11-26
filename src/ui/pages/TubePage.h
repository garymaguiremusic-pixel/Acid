#pragma once

#include "PageBase.h"
#include "../TubeView.h"
#include "../arp/Controls.h"

class TubePage : public PageBase
{
public:
    explicit TubePage (AcidAudioProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void update() override;

private:
    // the Saturate / Foldback distortion (top)
    juce::TextButton distOnButton { "ON" };
    juce::OwnedArray<juce::TextButton> modeButtons;     // the six modes, each on / off
    AcidKnob distDrive { AcidKnob::Size::Small, "Drive" }, distAmount { AcidKnob::Size::Small, "Distort" };

    // the tube (one band)
    juce::TextButton tubeOnButton { "ON" };
    juce::TextButton generateButton { "GENERATE DISTORTION" }, undoButton { "UNDO" }, initButton { "INIT" }, redoButton { "REDO" };
    AcidKnob resonator { AcidKnob::Size::Small, "Resonator" }, character { AcidKnob::Size::Small, "Character" };
    AcidKnob input { AcidKnob::Size::Small, "Input" }, output { AcidKnob::Size::Small, "Output" }, mix { AcidKnob::Size::Small, "Mix" };

    AcidKnob tubeDrive { AcidKnob::Size::Small, "Drive" };     // one big Drive (a single band)
    TubeView tube;
    InfoLcd tubeDriveLcd;

    // the delay (bottom of the page)
    juce::TextButton delayOn { "ON" }, delaySync { "Sync" };
    AcidKnob delayTime { AcidKnob::Size::Small, "Time" }, delayMix { AcidKnob::Size::Small, "Mix" }, delayFeedback { AcidKnob::Size::Small, "Feedback" };
    AcidKnob delayLowCut { AcidKnob::Size::Small, "Low Cut" }, delayHighCut { AcidKnob::Size::Small, "High Cut" }, delayWidth { AcidKnob::Size::Small, "Width" };
    InfoLcd delayLcd;
    static constexpr int kDelayH = 122;

};
