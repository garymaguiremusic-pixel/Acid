#pragma once

#include "PageBase.h"
#include "SequencerPage.h"
#include "LayerRow.h"
#include "TubePage.h"
#include "../arp/Controls.h"
#include "Page303.h"

/** The one page: a compact section on top (A = Acid, B = 303, FX = distortion, tube and
    delay), chosen with the tabs, and the sequencer always underneath. */
class SynthPage : public PageBase
{
public:
    static constexpr int kSectionH = 240;      // two rows of 117

    explicit SynthPage (AcidAudioProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void update() override;

    /** The section shown over the sequencer: 0 = A (Acid), 1 = B (303), 2 = FX. */
    void setView (int view);

private:
    // A, row 1: Osc 1 | Filter | Envelope
    juce::TextButton waveButton { "SAW" }, retroButton { "Retro" }, lockButton { "Phase Lock" };
    WaveDisplay waveDisplay;
    OctaveStepper octaveA;
    AcidKnob cutoffKnob { AcidKnob::Size::Small, "Cutoff" }, resoKnob { AcidKnob::Size::Small, "Resonance" };
    AcidKnob cutoffLowKnob { AcidKnob::Size::Small, "Cut Low" }, driveKnob { AcidKnob::Size::Small, "Drive" };
    AcidKnob bassKnob { AcidKnob::Size::Small, "Bass" }, trebleKnob { AcidKnob::Size::Small, "Treble" };
    juce::TextButton transistorButton { "Transistor" };
    SegmentedButtons filterModeButtons;     // LP / BP / HP
    AcidKnob envModKnob { AcidKnob::Size::Small, "Env Mod" }, decayKnob { AcidKnob::Size::Small, "Decay" };
    AcidKnob accentKnob { AcidKnob::Size::Small, "Accent" };
    AcidKnob envAttackKnob { AcidKnob::Size::Small, "Attack" }, accDecayKnob { AcidKnob::Size::Small, "Acc Dec" };
    AcidKnob accSweepKnob { AcidKnob::Size::Small, "Sweep" };
    std::vector<juce::Component*> acidControls;

    LayerRow layerRow;            // A, row 2: Osc 2, Sync / FM, Unison, Velocity, Amp Envelope
    Page303 page303;              // B: the 303 and its options
    AmpEnvPanel amp303;           // B: the 303's own Amp Envelope
    TubePage fx;                  // FX: distortion, delay, tube

    int view = 0, shownView = -1;
    SequencerPage sequencer;      // always underneath
};
