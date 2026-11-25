#pragma once

#include "PageBase.h"
#include "../arp/Controls.h"

/** The 303 section: Waveform, Octave and Options, then the seven 303 knobs in one row.
    The extra settings (VCO model, gate, noise, aged parts ...) are in the Options menu. */
class Page303 : public PageBase
{
public:
    explicit Page303 (AcidAudioProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void showOptions();

    SegmentedButtons waveButtons;
    OctaveStepper octave;
    juce::TextButton optionsButton { juce::CharPointer_UTF8 ("Options \xe2\x96\xbe") };
    AcidKnob tuning { AcidKnob::Size::Small, "Tuning" }, cutoff { AcidKnob::Size::Small, "Cutoff" },
             resonance { AcidKnob::Size::Small, "Resonance" }, envMod { AcidKnob::Size::Small, "Env Mod" },
             decay { AcidKnob::Size::Small, "Decay" }, accent { AcidKnob::Size::Small, "Accent" },
             volume { AcidKnob::Size::Small, "Volume" };
};
