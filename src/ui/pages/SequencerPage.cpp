#include "SequencerPage.h"

using namespace juce;
using namespace acid;

namespace
{
    const Rectangle<float> kArpPanel    { 8.0f, 0.0f, 380.0f, 266.0f };
    const Rectangle<float> kRandomPanel { 394.0f, 0.0f, 598.0f, 266.0f };
    constexpr int kStripY = 272, kSeqY = 306;
}

SequencerPage::SequencerPage (AcidAudioProcessor& p)
    : PageBase (p),
      rateTypeButtons (*p.apvts.getParameter (ParamIDs::arpRateType), as::getRateTypeNames(), "Straight, triplet or dotted note values"),
      octaveButtons (*p.apvts.getParameter (ParamIDs::arpOctave), { "1", "2", "3", "4" },
                     "Octave range: each full cycle repeats one octave higher, up to this many octaves"),
      tempoField (*p.apvts.getParameter (ParamIDs::bpm)),
      seqPanel (p),
      sequenceBrowser (p.getSequenceLibrary(), p.getSequenceBrowserState(), "*.mid;*.midi;" + SeqPanel::audioWildcard(), nullptr,
                       { "Sequence Browser", "file", "files", false })
{
    using namespace ParamIDs;

    // Arpeggiator ---------------------------------------------------------------------
    attach (arpOnButton, arpOn);
    attach (autoPlayButton, autoPlay);
    attach (midiOutButton, midiOut);
    attach (modeBox, arpMode);
    attach (rateBox, arpRate);
    attach (velModeBox, velMode);
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

    const std::pair<AcidKnob*, const char*> knobs[] = { { &densityKnob, rndDensity }, { &rangeKnob, rndRange }, { &tieKnob, rndTieProb },
                                                        { &ratchetKnob, rndRatchet }, { &accentKnob, rndAccent },
                                                        { &velLoKnob, rndVelMin }, { &velHiKnob, rndVelMax }, { &glideKnob, rndGlide } };
    for (auto [knob, id] : knobs)
        attach (*knob, id);

    const std::pair<TextButton*, const char*> locks[] = { { &lockPitchButton, rndLockPitch }, { &lockVelButton, rndLockVel },
                                                          { &lockRestButton, rndLockRest }, { &lockTieButton, rndLockTie },
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

    // Sequence Browser ----------------------------------------------------------------
    addAndMakeVisible (sequenceLcd);
    setHelp (sequenceLcd, "SEQUENCE  -  the MIDI or audio file the steps were loaded from");
    favouriteButton.onClick = [this]
    {
        const auto file = processor.getCurrentSequenceFile();
        if (file.existsAsFile())
            processor.getSequenceLibrary().toggleFavourite (file);
    };
    setHelp (favouriteButton, "FAVOURITE  -  add the loaded sequence file to your favourites (or remove it)");
    backButton.onClick = [this] { sequenceBrowser.step (-1); };
    nextButton.onClick = [this] { sequenceBrowser.step (+1); };
    setHelp (backButton, "PREVIOUS  -  the previous MIDI / audio file in the Sequence Browser, loaded into the steps in your Key / Scale");
    setHelp (nextButton, "NEXT  -  the next MIDI / audio file in the Sequence Browser, loaded into the steps in your Key / Scale");
    browseButton.setClickingTogglesState (true);
    browseButton.onClick = [this] { setSequenceBrowserVisible (browseButton.getToggleState()); };
    setHelp (browseButton, "BROWSE  -  MIDI and audio files: each one you pick loads into the steps, in your Key / Scale");
    for (Component* c : { (Component*) &favouriteButton, (Component*) &backButton, (Component*) &nextButton, (Component*) &browseButton })
        addAndMakeVisible (c);

    sequenceBrowser.onLoad = [this] (const File& f) { seqPanel.importFile (f, true); };
    sequenceBrowser.getCurrentSample = [this] { return processor.getCurrentSequenceFile(); };
    sequenceBrowser.onClose = [this] { setSequenceBrowserVisible (false); };

    addAndMakeVisible (seqPanel);
    addChildComponent (sequenceBrowser);
    setSequenceBrowserVisible (processor.isSequenceBrowserOpen());
}

void SequencerPage::setSequenceBrowserVisible (bool shouldShow)
{
    processor.setSequenceBrowserOpen (shouldShow);
    browseButton.setToggleState (shouldShow, dontSendNotification);
    sequenceBrowser.setVisible (shouldShow);
    if (shouldShow)
        sequenceBrowser.toFront (false);
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
        label ("Velocity", 222, 183, 100);
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
    modeBox.setBounds (ax + 92, ay + 40, 120, 26);
    octaveButtons.setBounds (ax + 220, ay + 40, 148, 26);
    rateBox.setBounds (ax + 10, ay + 92, 74, 26);
    rateTypeButtons.setBounds (ax + 92, ay + 92, 276, 26);
    gateKnob.setBounds (ax + 14, ay + 126, 84, 84);
    swingKnob.setBounds (ax + 116, ay + 126, 84, 84);
    gateLcd.setBounds (ax + 10, ay + 216, 94, 26);
    swingLcd.setBounds (ax + 112, ay + 216, 94, 26);
    autoPlayButton.setBounds (ax + 220, ay + 126, 148, 24);
    midiOutButton.setBounds (ax + 220, ay + 154, 148, 24);
    velModeBox.setBounds (ax + 220, ay + 198, 148, 24);
    tempoField.setBounds (ax + 220, ay + 230, 148, 26);

    // Randomise
    const int rx = (int) kRandomPanel.getX(), ry = (int) kRandomPanel.getY();
    randomiseButton.setBounds (rx + 10, ry + 28, 280, 28);
    randomUndoButton.setBounds (rx + 298, ry + 28, 290, 28);
    AcidKnob* rndKnobs[] = { &densityKnob, &rangeKnob, &glideKnob, &tieKnob, &ratchetKnob, &accentKnob, &velLoKnob, &velHiKnob };
    for (int i = 0; i < 8; ++i)
        rndKnobs[i]->setBounds (rx + 10 + i * 72, ry + 62, 70, 76);
    TextButton* locks[] = { &lockPitchButton, &lockVelButton, &lockRestButton, &lockGlideButton, &lockTieButton, &lockRatchetButton, &lockAccentButton };
    for (int i = 0; i < 7; ++i)
        locks[i]->setBounds (rx + 52 + i * 76, ry + 142, 72, 22);
    scaleLockButton.setBounds (rx + 10, ry + 174, 110, 26);
    keyBox.setBounds (rx + 128, ry + 174, 70, 26);
    scaleBox.setBounds (rx + 206, ry + 174, 180, 26);
    fileNotesButton.setBounds (rx + 394, ry + 174, 194, 26);
    dragMidi.setBounds (rx + 10, ry + 226, 120, 30);
    exportBarsBox.setBounds (rx + 196, ry + 230, 110, 24);
    exportRootBox.setBounds (rx + 314, ry + 230, 110, 24);
    exportChordBox.setBounds (rx + 432, ry + 230, 156, 24);

    // Sequence Browser strip and its browser (over the panels)
    sequenceLcd.setBounds (8, kStripY, 754, 28);
    favouriteButton.setBounds (768, kStripY, 28, 28);
    backButton.setBounds (802, kStripY, 28, 28);
    nextButton.setBounds (834, kStripY, 28, 28);
    browseButton.setBounds (868, kStripY, 124, 28);
    sequenceBrowser.setBounds (8, 0, 984, kSeqY - 4);     // over the panels and the strip (Close shuts it)

    seqPanel.setBounds (8, kSeqY, 984, getHeight() - kSeqY);
}

void SequencerPage::update()
{
    seqPanel.update();
    gateLcd.setContent ("GATE", describeParameter (ParamIDs::arpGate, value (ParamIDs::arpGate)));
    swingLcd.setContent ("SWING", describeParameter (ParamIDs::arpSwing, value (ParamIDs::arpSwing)));
    tempoField.setTempo (processor.getTempo(), processor.isTempoFromHost());

    const auto sequence = processor.getCurrentSequenceFile();
    const bool haveSequence = sequence != File();
    sequenceLcd.setContent ("SEQUENCE", haveSequence ? sequence.getFileNameWithoutExtension().toUpperCase()
                                                     : String ("BROWSE OR DROP A MIDI / AUDIO FILE"));
    sequenceLcd.setDimmed (! haveSequence);
    favouriteButton.setEnabled (haveSequence);
    favouriteButton.setToggleState (haveSequence && processor.getSequenceLibrary().isFavourite (sequence), dontSendNotification);
    backButton.setEnabled (sequenceBrowser.hasFiles());
    nextButton.setEnabled (sequenceBrowser.hasFiles());

    const int poolNotes = processor.getPatternModel().get().numPoolNotes();
    fileNotesButton.setButtonText (poolNotes > 0 ? "File Notes (" + String (poolNotes) + ")" : String ("File Notes"));
    fileNotesButton.setEnabled (poolNotes > 0);
    exportChordBox.setEnabled (processor.getLastHeldNotes().empty());
    randomUndoButton.setEnabled (processor.getPatternModel().canUndo());
}
