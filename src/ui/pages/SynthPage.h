#pragma once

#include "PageBase.h"
#include "SequencerPage.h"
#include "../arp/Controls.h"

class SynthPage : public PageBase
{
public:
    explicit SynthPage (AcidAudioProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void update() override;

private:
    // OSC
    juce::TextButton waveButton { "SAW" }, retroButton { "Retro" }, lockButton { "Phase Lock" };
    WaveDisplay waveDisplay;

    // FILTER
    AcidKnob cutoffKnob { AcidKnob::Size::Large, "Cutoff" }, resoKnob { AcidKnob::Size::Large, "Resonance" };
    AcidKnob cutoffLowKnob { AcidKnob::Size::Small, "Cutoff Low" }, driveKnob { AcidKnob::Size::Small, "Drive" };
    AcidKnob bassKnob { AcidKnob::Size::Small, "Bass" }, trebleKnob { AcidKnob::Size::Small, "Treble" };
    juce::TextButton transistorButton { "Transistor" };
    InfoLcd cutoffLcd, resoLcd;
    SegmentedButtons filterModeButtons;     // LP / BP / HP, beside the cutoff

    // FILTER > VELOCITY: velocity (the sequencer's Velocity lane) moves the cutoff
    juce::TextButton velLinkButton { "Vel Link" };
    AcidKnob velAmountKnob { AcidKnob::Size::Small, "Vel Amt" };
    InfoLcd velocityLcd;

    // ENVELOPE
    AcidKnob envModKnob { AcidKnob::Size::Small, "Env Mod" }, decayKnob { AcidKnob::Size::Small, "Decay" };
    AcidKnob accentKnob { AcidKnob::Size::Small, "Accent" };
    AcidKnob envAttackKnob { AcidKnob::Size::Small, "Env Attack" }, accDecayKnob { AcidKnob::Size::Small, "Acc Decay" };
    AcidKnob accSweepKnob { AcidKnob::Size::Small, "Acc Sweep" };
    EnvMeter envMeter;

    // The sequencer (from ArpSampler): Arpeggiator, Randomise, Sequence Browser, Step Sequencer
    SequencerPage sequencer;
};
