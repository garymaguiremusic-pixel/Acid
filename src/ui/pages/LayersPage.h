#pragma once

#include "PageBase.h"
#include "../arp/Controls.h"

class LayersPage : public PageBase
{
public:
    explicit LayersPage (AcidAudioProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void update() override;

private:
    struct LfoControls
    {
        AcidKnob rate { AcidKnob::Size::Small, "Rate" }, depth { AcidKnob::Size::Small, "Depth" }, phase { AcidKnob::Size::Small, "Phase" };
        LfoPreview preview;
        juce::TextButton prevShape { juce::CharPointer_UTF8 ("\xe2\x97\x80") }, nextShape { juce::CharPointer_UTF8 ("\xe2\x96\xb6") };
        juce::TextButton sync { "Sync" }, mirror { "Mirror" };
        juce::ComboBox retrig;
        std::array<juce::TextButton, 5> dest;
        InfoLcd readout;
    };

    void setupLfo (int index);
    void layoutLfo (int index, int x0);

    juce::TextButton oscBLockButton { "Phase Lock" }, syncButton { "Hard Sync" };
    AcidKnob fmAmount { AcidKnob::Size::Small, "FM" };
    SegmentedButtons fmDirection;
    OctaveStepper octaveB;
    AcidKnob oscBLevel { AcidKnob::Size::Small, "Level" }, oscBTune { AcidKnob::Size::Small, "Tune" }, oscBDetune { AcidKnob::Size::Small, "Detune" };
    juce::TextButton unisonFree { "Free" };
    AcidKnob voices { AcidKnob::Size::Small, "Voices" }, spread { AcidKnob::Size::Small, "Spread" };
    AcidKnob ampAttack { AcidKnob::Size::Small, "Attack" }, ampDecay { AcidKnob::Size::Small, "Decay" }, ampRelease { AcidKnob::Size::Small, "Release" };
    InfoLcd ampLcd;

    juce::TextButton echoSync { "Sync" };
    AcidKnob echoTime { AcidKnob::Size::Small, "Time" }, echoMix { AcidKnob::Size::Small, "Mix" }, echoFeedback { AcidKnob::Size::Small, "Feedback" };
    AcidKnob echoLowCut { AcidKnob::Size::Small, "Low Cut" }, echoHighCut { AcidKnob::Size::Small, "High Cut" }, echoWidth { AcidKnob::Size::Small, "Width" };
    InfoLcd echoLcd;

    LfoControls lfo[2];
};
