#include "SequencerPage.h"

using namespace juce;
using namespace acid;

namespace
{
    const Rectangle<float> kArpPanel    { 8.0f, 0.0f, 380.0f, 266.0f };
    const Rectangle<float> kRandomPanel { 394.0f, 0.0f, 598.0f, 266.0f };
    constexpr int kSeqY = 272;
}

SequencerPage::SequencerPage (AcidAudioProcessor& p)
    : PageBase (p),
      rateTypeButtons (*p.apvts.getParameter (ParamIDs::arpRateType), as::getRateTypeNames(), "Straight, triplet or dotted note values"),
      octaveButtons (*p.apvts.getParameter (ParamIDs::arpOctave), { "1", "2", "3", "4" },
                     "Octave range: each full cycle repeats one octave higher, up to this many octaves"),
      tempoField (*p.apvts.getParameter (ParamIDs::bpm)),
      seqPanel (p)
{
    using namespace ParamIDs;

    // Arpeggiator ---------------------------------------------------------------------
    attach (arpOnButton, arpOn);
    attach (midiOutButton, midiOut);
    // Mode: Step only (the step sequencer plays the pattern)
    modeStep.setToggleState (true, dontSendNotification);
    modeStep.setInterceptsMouseClicks (false, false);     // the only mode: shown, not a choice
    setHelp (modeStep, "MODE  -  Step: the step sequencer plays the pattern");
    addAndMakeVisible (modeStep);
    attach (rateBox, arpRate);
    addAndMakeVisible (rateTypeButtons);
    addAndMakeVisible (octaveButtons);
    setHelp (rateTypeButtons, "TIME  -  " + helpForParameter (arpRateType));
    setHelp (octaveButtons, "OCTAVE  -  " + helpForParameter (arpOctave));
    attach (gateKnob, arpGate);
    attach (swingKnob, arpSwing);
    addAndMakeVisible (gateLcd);
    addAndMakeVisible (swingLcd);
    addAndMakeVisible (tempoField);

    // Randomise -----------------------------------------------------------------------
    randomiseButton.onClick = [this] { processor.randomisePattern(); };
    setHelp (randomiseButton, "RANDOMISE  -  a new step pattern (undoable). Locked lanes are kept.");
    randomUndoButton.onClick = [this] { processor.getPatternModel().undo(); };
    setHelp (randomUndoButton, "UNDO  -  back to the pattern before the last randomise or edit");
    addAndMakeVisible (randomiseButton);
    addAndMakeVisible (randomUndoButton);

    const std::pair<AcidKnob*, const char*> knobs[] = { { &densityKnob, rndDensity }, { &rangeKnob, rndRange },
                                                        { &ratchetKnob, rndRatchet }, { &accentKnob, rndAccent },
                                                        { &velLoKnob, rndVelMin }, { &velHiKnob, rndVelMax }, { &glideKnob, rndGlide } };
    for (auto [knob, id] : knobs)
        attach (*knob, id);

    const std::pair<TextButton*, const char*> locks[] = { { &lockPitchButton, rndLockPitch }, { &lockVelButton, rndLockVel },
                                                          { &lockRestButton, rndLockRest },
                                                          { &lockRatchetButton, rndLockRatchet }, { &lockAccentButton, rndLockAccent },
                                                          { &lockGlideButton, rndLockGlide } };
    for (auto [button, id] : locks)
        attach (*button, id);

    attach (scaleLockButton, ParamIDs::scaleLock);
    attach (fileNotesButton, rndFileNotes);
    attach (keyBox, ParamIDs::scaleKey);
    attach (scaleBox, ParamIDs::scale);

    // MIDI export
    dragMidi.createFile = [this] { return processor.exportMidiToTempFile(); };
    setHelp (dragMidi, "DRAG MIDI  -  drag into your DAW to drop the pattern as a MIDI clip (right-click to save a file)");
    addAndMakeVisible (dragMidi);
    attach (exportBarsBox, exportBars);
    attach (exportChordBox, exportChord);
    exportRootBox.addItem ("Auto", 1);
    for (int n = 0; n < 128; ++n)
        exportRootBox.addItem (as::noteName (n), n + 2);
    exportRootAttachment = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, exportRoot, exportRootBox);
    setHelp (exportRootBox, "EXPORT ROOT  -  " + helpForParameter (exportRoot));
    addAndMakeVisible (exportRootBox);

    addAndMakeVisible (seqPanel);
}

void SequencerPage::paint (Graphics& g)
{
    AcidLookAndFeel::drawPanel (g, kArpPanel, "Arpeggiator");
    {
        const float ax = kArpPanel.getX(), ay = kArpPanel.getY();
        auto label = [&] (const char* text, float x, float y, float w)
        {
            AcidLookAndFeel::drawSmallLabel (g, text, { ax + x, ay + y, w, 14.0f }, Justification::centredLeft);
        };
        label ("Arp", 12, 24, 70);
        label ("Mode", 94, 24, 110);
        label ("Octave", 222, 24, 100);
        label ("Time", 12, 76, 70);
    }

    AcidLookAndFeel::drawPanel (g, kRandomPanel, "Randomise");
    AcidLookAndFeel::drawSmallLabel (g, "Lock", { kRandomPanel.getX() + 12.0f, kRandomPanel.getY() + 146.0f, 44.0f, 14.0f }, Justification::centredLeft);
    AcidLookAndFeel::drawSmallLabel (g, "Export", { kRandomPanel.getX() + 140.0f, kRandomPanel.getY() + 214.0f, 60.0f, 14.0f }, Justification::centredLeft);
}

void SequencerPage::resized()
{
    // Arpeggiator
    const int ax = (int) kArpPanel.getX(), ay = (int) kArpPanel.getY();
    arpOnButton.setBounds (ax + 10, ay + 40, 74, 26);
    modeStep.setBounds (ax + 92, ay + 40, 120, 26);
    octaveButtons.setBounds (ax + 220, ay + 40, 148, 26);
    rateBox.setBounds (ax + 10, ay + 92, 74, 26);
    rateTypeButtons.setBounds (ax + 92, ay + 92, 276, 26);
    gateKnob.setBounds (ax + 14, ay + 126, 84, 84);
    swingKnob.setBounds (ax + 116, ay + 126, 84, 84);
    gateLcd.setBounds (ax + 10, ay + 216, 94, 26);
    swingLcd.setBounds (ax + 112, ay + 216, 94, 26);
    midiOutButton.setBounds (ax + 220, ay + 152, 148, 26);
    tempoField.setBounds (ax + 220, ay + 216, 148, 26);

    // Randomise
    const int rx = (int) kRandomPanel.getX(), ry = (int) kRandomPanel.getY();
    randomiseButton.setBounds (rx + 10, ry + 28, 280, 28);
    randomUndoButton.setBounds (rx + 298, ry + 28, 290, 28);
    AcidKnob* rndKnobs[] = { &densityKnob, &rangeKnob, &glideKnob, &ratchetKnob, &accentKnob, &velLoKnob, &velHiKnob };
    for (int i = 0; i < 7; ++i)
        rndKnobs[i]->setBounds (rx + 10 + i * 82, ry + 62, 70, 76);
    TextButton* locks[] = { &lockPitchButton, &lockVelButton, &lockRestButton, &lockGlideButton, &lockRatchetButton, &lockAccentButton };
    for (int i = 0; i < 6; ++i)
        locks[i]->setBounds (rx + 52 + i * 90, ry + 142, 72, 22);
    scaleLockButton.setBounds (rx + 10, ry + 174, 110, 26);
    keyBox.setBounds (rx + 128, ry + 174, 70, 26);
    scaleBox.setBounds (rx + 206, ry + 174, 180, 26);
    fileNotesButton.setBounds (rx + 394, ry + 174, 194, 26);
    dragMidi.setBounds (rx + 10, ry + 226, 120, 30);
    exportBarsBox.setBounds (rx + 196, ry + 230, 110, 24);
    exportRootBox.setBounds (rx + 314, ry + 230, 110, 24);
    exportChordBox.setBounds (rx + 432, ry + 230, 156, 24);

    seqPanel.setBounds (8, kSeqY, 984, getHeight() - kSeqY);
}

void SequencerPage::update()
{
    seqPanel.update();
    gateLcd.setContent ("GATE", describeParameter (ParamIDs::arpGate, value (ParamIDs::arpGate)));
    swingLcd.setContent ("SWING", describeParameter (ParamIDs::arpSwing, value (ParamIDs::arpSwing)));
    tempoField.setTempo (processor.getTempo(), processor.isTempoFromHost());

    const int poolNotes = processor.getPatternModel().get().numPoolNotes();
    fileNotesButton.setButtonText (poolNotes > 0 ? "File Notes (" + String (poolNotes) + ")" : String ("File Notes"));
    fileNotesButton.setEnabled (poolNotes > 0);
    exportChordBox.setEnabled (processor.getLastHeldNotes().empty());
    randomUndoButton.setEnabled (processor.getPatternModel().canUndo());
}
