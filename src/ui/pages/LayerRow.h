#pragma once

#include "PageBase.h"
#include "AmpEnvPanel.h"
#include "../arp/Controls.h"

/** Section A's second row (compact): Osc 2, Sync / FM, Unison, Velocity and A's Amp Envelope. */
class LayerRow : public PageBase
{
public:
    static constexpr int kHeight = 117;

    explicit LayerRow (AcidAudioProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void update() override;

private:
    AcidKnob oscBLevel { AcidKnob::Size::Small, "Level" }, oscBTune { AcidKnob::Size::Small, "Tune" },
             oscBDetune { AcidKnob::Size::Small, "Detune" };
    OctaveStepper octaveB;
    juce::TextButton oscBLockButton { "Phase Lock" };

    juce::TextButton syncButton { "Hard Sync" };
    AcidKnob syncPitch { AcidKnob::Size::Small, "Sync" }, fmAmount { AcidKnob::Size::Small, "FM" };
    SegmentedButtons fmDirection;

    AcidKnob voices { AcidKnob::Size::Small, "Voices" }, spread { AcidKnob::Size::Small, "Spread" };
    juce::TextButton unisonFree { "Free" };

    juce::TextButton velLinkButton { "Vel Link" };
    AcidKnob velAmount { AcidKnob::Size::Small, "Amount" };

    AmpEnvPanel amp;
};
