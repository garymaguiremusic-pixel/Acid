#include "SeqPanel.h"
#include "../../arp/import/AudioToNotes.h"

using namespace juce;
using namespace acid;

SeqPanel::SeqPanel (AcidAudioProcessor& p)
    : PageBase (p),
      grid (p.getPatternModel(), *p.apvts.getParameter (ParamIDs::seqLength), *p.apvts.getParameter (ParamIDs::seqEnd)),
      lengthButtons (*p.apvts.getParameter (ParamIDs::seqLength), as::getSeqLengthNames(), "Number of steps")
{
    setInterceptsMouseClicks (false, true);
    addAndMakeVisible (grid);
    addAndMakeVisible (lengthButtons);
    setHelp (lengthButtons, "LENGTH  -  " + helpForParameter (ParamIDs::seqLength));
    attach (endBox, ParamIDs::seqEnd);
    attach (glideKnob, ParamIDs::slideTime);
    addAndMakeVisible (glideLcd);
    setHelp (glideLcd, "GLIDE  -  " + helpForParameter (ParamIDs::slideTime));

    auto& model = processor.getPatternModel();
    undoButton.onClick = [&model] { model.undo(); };
    redoButton.onClick = [&model] { model.redo(); };
    clearButton.onClick = [this, &model] { model.set (processor.makeEmptyPattern(), true); };
    importButton.onClick = [this] { makeImportMenu().showMenuAsync (PopupMenu::Options().withTargetComponent (&importButton)); };
    setHelp (undoButton, "UNDO  -  undo the last pattern change (edit, randomise, import or preset)");
    setHelp (redoButton, "REDO  -  redo a pattern change");
    setHelp (clearButton, "CLEAR  -  every step back to full volume, velocity 100, no transpose, no tie");
    setHelp (importButton, "IMPORT  -  a MIDI file (notes only) or the notes of an audio file into the steps. You can also drop a file on the grid.");
    for (auto* b : { &undoButton, &redoButton, &clearButton, &importButton })
        addAndMakeVisible (*b);

    grid.canImport = [] (const File& f) { return isMidiFile (f) || isAudioFile (f); };
    grid.onFileDropped = [this] (const File& f) { importFile (f); };
    grid.quantiseTranspose = [this] (int t) { return processor.quantiseEditedTranspose (t); };
}

PopupMenu SeqPanel::makeImportMenu()
{
    auto& choices = processor.getImportChoices();

    PopupMenu bars;
    for (int b = 1; b <= 8; ++b)
        bars.addItem ("Bar " + String (b), true, choices.startBar == b, [&choices, b] { choices.startBar = b; });

    PopupMenu menu;
    menu.addSectionHeader ("Import into the step sequencer");
    menu.addItem ("Import MIDI File...", [this] { chooseImportFile (false); });
    menu.addItem ("Detect Notes from Audio...", [this] { chooseImportFile (true); });
    menu.addSeparator();
    menu.addItem ("Chords: keep the top note", true, choices.topNote, [&choices] { choices.topNote = true; });
    menu.addItem ("Chords: keep the bottom note", true, ! choices.topNote, [&choices] { choices.topNote = false; });
    menu.addSubMenu ("Start at", bars);
    return menu;
}

void SeqPanel::chooseImportFile (bool audio)
{
    chooser = std::make_unique<FileChooser> (audio ? "Detect notes from audio" : "Import MIDI file",
                                             File::getSpecialLocation (File::userMusicDirectory),
                                             audio ? audioWildcard() : String ("*.mid;*.midi"));
    chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this] (const FileChooser& fc)
    {
        if (const auto file = fc.getResult(); file.existsAsFile())
            importFile (file);
    });
}

void SeqPanel::importFile (const File& file, bool lockToScale)
{
    if (! file.existsAsFile())
        return;

    const auto generation = ++importGeneration;     // a newer import makes slower ones still running stale

    if (isMidiFile (file))
    {
        MidiFile midi;
        FileInputStream in (file);
        if (! in.openedOk() || ! midi.readFrom (in))
        {
            grid.showStatus ("Couldn't read " + file.getFileName());
            return;
        }
        finishImport (as::import::readMidiNotes (midi), file, false, lockToScale);
        return;
    }

    // audio: detect the notes on a background thread, then fill the steps here
    grid.showStatus ("Detecting notes in " + file.getFileName() + "...");
    const double tempo = processor.getTempo();
    Thread::launch ([file, tempo, generation, lockToScale, safe = Component::SafePointer<SeqPanel> (this)]
    {
        auto notes = as::import::detectNotesInFile (file, tempo);
        MessageManager::callAsync ([safe, notes = std::move (notes), file, generation, lockToScale]
        {
            if (safe != nullptr && safe->importGeneration == generation)
                safe->finishImport (notes, file, true, lockToScale);
        });
    });
}

void SeqPanel::finishImport (const std::vector<as::import::ImportedNote>& notes, const File& file, bool fromAudio, bool lockToScale)
{
    const auto name = file.getFileName();
    const int used = processor.applyImport (notes, lockToScale);
    if (used == 0)
    {
        grid.showStatus (notes.empty() ? (fromAudio ? "No notes found in " + name + " (it needs a single-note line)"
                                                    : "No notes in " + name)
                                       : "No notes in the chosen bars of " + name);
        return;
    }
    processor.setCurrentSequenceFile (file);
    grid.showStatus ("Loaded " + String (used) + (used == 1 ? " note" : " notes") + " from " + name + "  (Undo to go back)");
}

void SeqPanel::paint (Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    AcidLookAndFeel::drawPanel (g, b, "Step Sequencer");
    AcidLookAndFeel::drawSmallLabel (g, "Length", { 12.0f, 24.0f, 70.0f, 14.0f }, Justification::centredLeft);
    AcidLookAndFeel::drawSmallLabel (g, "End", { 88.0f, 24.0f, 60.0f, 14.0f }, Justification::centredLeft);
    g.setColour (Theme::get().bevelDark);
    g.drawVerticalLine (kLeftWidth, 24.0f, b.getBottom() - 8.0f);
}

void SeqPanel::resized()
{
    // left column: Length / End, then Glide (knob + readout), then the edit buttons
    lengthButtons.setBounds (10, 40, 72, 22);
    endBox.setBounds (88, 40, 62, 22);
    glideKnob.setBounds (10, 68, 72, 80);
    undoButton.setBounds (90, 76, 60, 24);
    redoButton.setBounds (90, 106, 60, 24);
    glideLcd.setBounds (10, 152, 140, 24);
    const int h = getHeight();
    clearButton.setBounds (10, h - 34, 68, 24);
    importButton.setBounds (82, h - 34, 68, 24);
    grid.setBounds (kLeftWidth + 8, 22, getWidth() - kLeftWidth - 18, h - 30);
}

void SeqPanel::update()
{
    glideLcd.setContent ("GLIDE", describeParameter (ParamIDs::slideTime, value (ParamIDs::slideTime)));
    grid.setCurrentStep (processor.getCurrentStep());
    undoButton.setEnabled (processor.getPatternModel().canUndo());
    redoButton.setEnabled (processor.getPatternModel().canRedo());
}
