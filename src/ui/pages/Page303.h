#pragma once

#include "PageBase.h"
#include "../arp/Controls.h"

/** Section B: the 303 row (Waveform, Octave, Velocity and the seven 303 knobs) and, in the
    second row beside its Amp Envelope, the 303 options as buttons. */
class Page303 : public PageBase
{
public:
    explicit Page303 (AcidAudioProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    SegmentedButtons waveButtons, modelButtons;
    OctaveStepper octave;
    juce::TextButton velocityButton { "Velocity" };
    AcidKnob tuning { AcidKnob::Size::Small, "Tuning" }, cutoff { AcidKnob::Size::Small, "Cutoff" },
             resonance { AcidKnob::Size::Small, "Resonance" }, envMod { AcidKnob::Size::Small, "Env Mod" },
             decay { AcidKnob::Size::Small, "Decay" }, accent { AcidKnob::Size::Small, "Accent" },
             volume { AcidKnob::Size::Small, "Volume" };
    juce::OwnedArray<juce::TextButton> options;     // Accurate Gate ... Soft Accent
};
