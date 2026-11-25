#pragma once

#include "PageBase.h"
#include "SeqPanel.h"
#include "../arp/Controls.h"
#include "../arp/VirusWidgets.h"
#include "../arp/DragMidiHandle.h"

/** ArpSampler's sequencer: the Arpeggiator and Randomise panels and the step sequencer. */
class SequencerPage : public PageBase
{
public:
    explicit SequencerPage (AcidAudioProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void update() override;

private:

    // Arpeggiator
    juce::TextButton arpOnButton { "Arp On" }, autoPlayButton { "Auto Play" }, midiOutButton { "MIDI Out" };
    juce::ComboBox modeBox, rateBox, velModeBox;
    SegmentedButtons rateTypeButtons, octaveButtons;
    AcidKnob gateKnob { AcidKnob::Size::Large, "Gate" }, swingKnob { AcidKnob::Size::Large, "Swing" };
    LcdDisplay gateLcd, swingLcd;
    TempoField tempoField;

    // Randomise
    juce::TextButton randomiseButton { "Randomise" }, randomUndoButton { "Undo" };
    AcidKnob densityKnob { AcidKnob::Size::Small, "Density" }, rangeKnob { AcidKnob::Size::Small, "Range" },
             tieKnob { AcidKnob::Size::Small, "Tie" }, ratchetKnob { AcidKnob::Size::Small, "Ratch" },
             accentKnob { AcidKnob::Size::Small, "Acc" }, velLoKnob { AcidKnob::Size::Small, "Vel Lo" },
             velHiKnob { AcidKnob::Size::Small, "Vel Hi" }, glideKnob { AcidKnob::Size::Small, "Glide" };
    juce::TextButton lockPitchButton { "Pitch" }, lockVelButton { "Vel" }, lockRestButton { "Rests" }, lockTieButton { "Ties" },
                     lockRatchetButton { "Ratch" }, lockAccentButton { "Acc" }, lockGlideButton { "Glide" };
    juce::TextButton scaleLockButton { "Scale Lock" }, fileNotesButton { "File Notes" };
    juce::ComboBox keyBox, scaleBox;
    DragMidiHandle dragMidi;
    juce::ComboBox exportBarsBox, exportRootBox, exportChordBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> exportRootAttachment;

    SeqPanel seqPanel;
};
