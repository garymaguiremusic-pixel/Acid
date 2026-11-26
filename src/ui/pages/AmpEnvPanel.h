#pragma once

#include "PageBase.h"

/** An Amp Envelope panel (Attack, Decay, Release and tempo Sync from 1/64 to 1/1). A has
    one (in the row under the Acid synth) and B has its own (under the 303). */
class AmpEnvPanel : public PageBase
{
public:
    AmpEnvPanel (AcidAudioProcessor&, const char* attackId, const char* decayId, const char* releaseId, const char* syncId);
    void paint (juce::Graphics&) override;
    void resized() override;
    void update() override;

private:
    juce::String text (const char* id, float v) const;

    const char *attackId, *decayId, *releaseId, *syncId;
    AcidKnob attack { AcidKnob::Size::Small, "Attack" }, decay { AcidKnob::Size::Small, "Decay" },
             release { AcidKnob::Size::Small, "Release" };
    juce::TextButton syncButton { "Sync" };
    InfoLcd lcd;
};
