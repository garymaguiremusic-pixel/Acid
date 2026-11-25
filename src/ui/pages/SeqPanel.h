#pragma once

#include "PageBase.h"
#include "../arp/ArpStepGrid.h"
#include "../arp/Controls.h"
#include "../../arp/import/PatternImport.h"

/** The step sequencer panel (from ArpSampler): Length, End, Glide, Undo / Redo, Clear,
    Import and the lane grid. Shown on the Synth page and on the Sequencer page. */
class SeqPanel : public PageBase
{
public:
    explicit SeqPanel (AcidAudioProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void update() override;

    /** Imports a MIDI file (notes only) or detects the notes of an audio file into the steps.
        'lockToScale': out-of-key notes move into the Key / Scale (the Sequence Browser). */
    void importFile (const juce::File&, bool lockToScale = false);

    static juce::String audioWildcard()   { return "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3"; }
    static bool isMidiFile (const juce::File& f)   { return f.hasFileExtension ("mid;midi"); }
    static bool isAudioFile (const juce::File& f)  { return f.hasFileExtension ("wav;aif;aiff;flac;ogg;mp3"); }

    ArpStepGrid grid;

private:
    static constexpr int kLeftWidth = 160;
    juce::PopupMenu makeImportMenu();
    void chooseImportFile (bool audio);
    void finishImport (const std::vector<as::import::ImportedNote>&, const juce::File&, bool fromAudio, bool lockToScale);

    SegmentedButtons lengthButtons;
    juce::ComboBox endBox;
    AcidKnob glideKnob { AcidKnob::Size::Small, "Glide" };
    InfoLcd glideLcd;
    juce::TextButton undoButton { "Undo" }, redoButton { "Redo" }, clearButton { "Clear" }, importButton { "Import" };
    std::unique_ptr<juce::FileChooser> chooser;
    juce::uint32 importGeneration = 0;
};
