#pragma once

#include "PageBase.h"
#include "../StepGrid.h"

/** Drag this button to drop the current pattern into the DAW as a MIDI clip; click it for the MIDI menu. */
class MidiDragButton : public juce::TextButton
{
public:
    using juce::TextButton::TextButton;
    std::function<juce::File()> createFile;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    bool dragStarted = false;
    void mouseDown (const juce::MouseEvent& e) override { dragStarted = false; juce::TextButton::mouseDown (e); }
};

class SynthPage : public PageBase
{
public:
    explicit SynthPage (AcidAudioProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void update() override;

    StepGrid grid;

private:
    void showRandomMenu();
    void showMidiMenu();
    void setGridPage (int page);

    // OSC
    juce::TextButton waveButton { "SAW" }, retroButton { "Retro" };
    WaveDisplay waveDisplay;

    // FILTER
    AcidKnob cutoffKnob { AcidKnob::Size::Large, "Cutoff" }, resoKnob { AcidKnob::Size::Large, "Resonance" };
    AcidKnob cutoffLowKnob { AcidKnob::Size::Small, "Cutoff Low" }, driveKnob { AcidKnob::Size::Small, "Drive" };
    AcidKnob bassKnob { AcidKnob::Size::Small, "Bass" }, trebleKnob { AcidKnob::Size::Small, "Treble" };
    juce::TextButton transistorButton { "Transistor" };
    InfoLcd cutoffLcd, resoLcd;

    // ENVELOPE
    AcidKnob envModKnob { AcidKnob::Size::Small, "Env Mod" }, decayKnob { AcidKnob::Size::Small, "Decay" };
    AcidKnob accentKnob { AcidKnob::Size::Small, "Accent" }, gateKnob { AcidKnob::Size::Small, "Gate" };
    AcidKnob envAttackKnob { AcidKnob::Size::Small, "Env Attack" }, accDecayKnob { AcidKnob::Size::Small, "Acc Decay" };
    AcidKnob accSweepKnob { AcidKnob::Size::Small, "Acc Sweep" };
    EnvMeter envMeter;

    // SEQUENCER
    InfoLcd seqLcd, scaleLcd;
    juce::TextButton playButton { "PLAY" }, seqButton { "Seq" }, keyButton { "Key" };
    AcidKnob patternKnob { AcidKnob::Size::Small, "Pattern" }, lengthKnob { AcidKnob::Size::Small, "Length" };
    AcidKnob swingKnob { AcidKnob::Size::Small, "Swing" }, slideKnob { AcidKnob::Size::Small, "Slide" };
    AcidKnob transposeKnob { AcidKnob::Size::Small, "Transpose" };
    juce::ComboBox rateBox, directionBox;
    juce::TextButton randomButton { juce::CharPointer_UTF8 ("RANDOM \xe2\x96\xbe") }, mutateButton { "MUTATE" }, clearButton { "CLEAR" };
    juce::TextButton rotateLeftButton { juce::CharPointer_UTF8 ("\xe2\x97\x80") }, rotateRightButton { juce::CharPointer_UTF8 ("\xe2\x96\xb6") };
    juce::TextButton copyButton { "COPY" }, pasteButton { "PASTE" }, undoButton { "UNDO" }, redoButton { "REDO" };
    MidiDragButton midiButton { juce::CharPointer_UTF8 ("MIDI \xe2\x96\xbe") };
    juce::TextButton followButton { "FOLLOW" };
    std::array<juce::TextButton, 4> pageButtons;

    int lastPatternVersion = -1;
    bool lengthDragging = false, lengthFirstEdit = false;
    std::unique_ptr<juce::FileChooser> chooser;
};
