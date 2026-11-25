#include "SynthPage.h"
#include "../../seq/MidiExport.h"

using namespace juce;
using namespace acid;

//==============================================================================
void MidiDragButton::mouseDrag (const MouseEvent& e)
{
    if (dragStarted || e.getDistanceFromDragStart() < 8 || ! createFile)
        return;
    dragStarted = true;
    setState (Button::buttonNormal);
    const auto file = createFile();
    if (file.existsAsFile())
        DragAndDropContainer::performExternalDragDropOfFiles ({ file.getFullPathName() }, false, this);
}

//==============================================================================
SynthPage::SynthPage (AcidAudioProcessor& p) : PageBase (p), grid (p)
{
    using namespace ParamIDs;

    // OSC
    waveButton.onClick = [this] { processor.setParameterValue (ParamIDs::wave, value (ParamIDs::wave) > 0.5f ? 0.0f : 1.0f); };
    setHelp (waveButton, "WAVE  -  " + helpForParameter (wave) + " Click to switch.");
    addAndMakeVisible (waveButton);
    addAndMakeVisible (waveDisplay);
    setHelp (waveDisplay, "WAVE  -  the oscillator waveform, drawn from the DSP");
    attach (retroButton, retro);

    // FILTER
    attach (cutoffKnob, cutoff);
    attach (resoKnob, resonance);
    attach (cutoffLowKnob, cutoffLow);
    attach (driveKnob, drive);
    attach (bassKnob, bass);
    attach (trebleKnob, treble);
    attach (transistorButton, transistor);
    addAndMakeVisible (cutoffLcd);
    addAndMakeVisible (resoLcd);

    // ENVELOPE
    attach (envModKnob, envMod);
    attach (decayKnob, decay);
    attach (accentKnob, accent);
    attach (gateKnob, ParamIDs::gate);
    attach (envAttackKnob, envAttack);
    attach (accDecayKnob, accDecay);
    attach (accSweepKnob, accSweep);
    addAndMakeVisible (envMeter);
    setHelp (envMeter, "ENV / ACC  -  the filter envelope and the accent-sweep charge, live");

    // SEQUENCER
    addAndMakeVisible (seqLcd);
    setHelp (seqLcd, "SEQUENCER  -  tempo, transport, pattern slot and length");
    playButton.setClickingTogglesState (false);
    playButton.onClick = [this] { processor.setInternalPlay (! processor.isInternalPlayOn()); };
    setHelp (playButton, "PLAY  -  run the pattern without the host (host playback always syncs)");
    addAndMakeVisible (playButton);
    attach (seqButton, seqOn);
    attach (keyButton, keyMode);

    attach (patternKnob, ParamIDs::pattern);
    attach (swingKnob, swing);
    attach (slideKnob, slideTime);
    attach (transposeKnob, transpose);

    // Length edits the current pattern (part of the pattern data, with undo)
    lengthKnob.setRange (1.0, (double) kMaxSteps, 1.0);
    lengthKnob.setDefaultValue (16.0);
    lengthKnob.displayValue = [] (double v) { return String (roundToInt (v)) + " steps"; };
    lengthKnob.helpText = "Pattern length (1 - 64 steps). Also: click a step number in the grid.";
    lengthKnob.onGestureBegin = [this] { lengthDragging = true; lengthFirstEdit = true; };
    lengthKnob.onGestureEnd = [this] { lengthDragging = false; };
    lengthKnob.onValueChange = [this]
    {
        const int len = roundToInt (lengthKnob.getValue());
        processor.editPattern (processor.getCurrentSlot(), [len] (Pattern& pat) { pat.length = len; }, lengthFirstEdit);
        lengthFirstEdit = false;
    };
    addAndMakeVisible (lengthKnob);

    attach (rateBox, seqRate);
    attach (directionBox, direction);

    randomButton.onClick = [this] { showRandomMenu(); };
    setHelp (randomButton, "RANDOM  -  new pattern (all, notes, accents, slides or octaves) in the chosen scale and root");
    mutateButton.onClick = [this] { processor.mutatePattern(); };
    setHelp (mutateButton, "MUTATE  -  small random changes to the pattern");
    clearButton.onClick = [this] { processor.clearPattern(); };
    setHelp (clearButton, "CLEAR  -  reset the pattern (keeps its length)");
    rotateLeftButton.onClick = [this] { processor.rotatePattern (-1); };
    setHelp (rotateLeftButton, "ROTATE LEFT  -  shift every step one place earlier");
    rotateRightButton.onClick = [this] { processor.rotatePattern (1); };
    setHelp (rotateRightButton, "ROTATE RIGHT  -  shift every step one place later");
    copyButton.onClick = [this] { processor.copyPattern(); };
    setHelp (copyButton, "COPY  -  copy this pattern");
    pasteButton.onClick = [this] { processor.pastePattern(); };
    setHelp (pasteButton, "PASTE  -  paste into this pattern slot");
    undoButton.onClick = [this] { processor.undoPattern(); };
    setHelp (undoButton, "UNDO  -  undo the last pattern edit");
    redoButton.onClick = [this] { processor.redoPattern(); };
    setHelp (redoButton, "REDO  -  redo a pattern edit");
    midiButton.onClick = [this] { showMidiMenu(); };
    midiButton.createFile = [this]
    {
        const auto& pat = processor.getPattern (processor.getCurrentSlot());
        return writePatternMidiFile (pat, roundToInt (value (ParamIDs::transpose)), roundToInt (value (seqRate)),
                                     value (ParamIDs::gate) / 100.0f,
                                     String (ACID_PRODUCT_NAME) + " Pattern P" + String (processor.getCurrentSlot() + 1).paddedLeft ('0', 2));
    };
    setHelp (midiButton, "MIDI  -  drag this button into your DAW to drop the pattern as a MIDI clip; click for export / import");
    followButton.setClickingTogglesState (true);
    followButton.setToggleState (true, dontSendNotification);
    setHelp (followButton, "FOLLOW  -  the grid page follows the playing step");

    for (auto* b : { &randomButton, &mutateButton, &clearButton, &rotateLeftButton, &rotateRightButton, &copyButton,
                     &pasteButton, &undoButton, &redoButton, (TextButton*) &midiButton, &followButton })
        addAndMakeVisible (*b);

    static const char* pageNames[] = { "1-16", "17-32", "33-48", "49-64" };
    for (int i = 0; i < 4; ++i)
    {
        auto& b = pageButtons[(size_t) i];
        b.setButtonText (pageNames[i]);
        b.onClick = [this, i] { followButton.setToggleState (false, dontSendNotification); setGridPage (i); };
        setHelp (b, String ("PAGE  -  show steps ") + pageNames[i]);
        addAndMakeVisible (b);
    }
    pageButtons[0].setToggleState (true, dontSendNotification);

    addAndMakeVisible (scaleLcd);
    setHelp (scaleLcd, "SCALE  -  root and scale used by Random and Mutate (set them in the Random menu)");
    addAndMakeVisible (grid);
}

void SynthPage::setGridPage (int page)
{
    grid.setPage (page);
    for (int i = 0; i < 4; ++i)
        pageButtons[(size_t) i].setToggleState (i == page, dontSendNotification);
}

//==============================================================================
void SynthPage::showRandomMenu()
{
    PopupMenu m;
    m.addSectionHeader ("Randomise");
    const char* modes[] = { "Everything", "Notes only", "Accents only", "Slides only", "Octaves only" };
    for (int i = 0; i < 5; ++i)
        m.addItem (i + 1, modes[i]);

    PopupMenu roots, scales;
    for (int r = 0; r < 12; ++r)
        roots.addItem (100 + r, PatternTools::rootName (r), true, processor.randomRoot == r);
    const auto& sc = PatternTools::scales();
    for (int s = 0; s < (int) sc.size(); ++s)
        scales.addItem (200 + s, sc[(size_t) s].name, true, processor.randomScale == s);
    m.addSeparator();
    m.addSubMenu ("Root: " + String (PatternTools::rootName (processor.randomRoot)), roots);
    m.addSubMenu ("Scale: " + String (sc[(size_t) processor.randomScale].name), scales);

    m.showMenuAsync (PopupMenu::Options().withTargetComponent (&randomButton), [this] (int r)
    {
        if (r >= 1 && r <= 5)            processor.randomisePattern ((PatternTools::RandomMode) (r - 1));
        else if (r >= 100 && r < 112)    processor.randomRoot = r - 100;
        else if (r >= 200 && r < 300)    processor.randomScale = r - 200;
    });
}

void SynthPage::showMidiMenu()
{
    PopupMenu m;
    m.addItem (1, "Export pattern as MIDI file...");
    m.addItem (2, "Import MIDI file into this pattern...");
    m.addSeparator();
    m.addItem (3, "Send sequencer notes to MIDI out", true, processor.midiOutEnabled.load());
    m.addSeparator();
    m.addItem (4, "Tip: drag the MIDI button into your DAW", false, false);

    m.showMenuAsync (PopupMenu::Options().withTargetComponent (&midiButton), [this] (int r)
    {
        if (r == 3)
        {
            processor.midiOutEnabled.store (! processor.midiOutEnabled.load());
        }
        else if (r == 1)
        {
            chooser = std::make_unique<FileChooser> ("Export pattern", File::getSpecialLocation (File::userDocumentsDirectory)
                                                         .getChildFile ("Pattern P" + String (processor.getCurrentSlot() + 1).paddedLeft ('0', 2) + ".mid"), "*.mid");
            chooser->launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::warnAboutOverwriting, [this] (const FileChooser& fc)
            {
                auto target = fc.getResult();
                if (target == File()) return;
                const auto temp = midiButton.createFile();
                temp.copyFileTo (target.withFileExtension ("mid"));
            });
        }
        else if (r == 2)
        {
            chooser = std::make_unique<FileChooser> ("Import MIDI", File::getSpecialLocation (File::userDocumentsDirectory), "*.mid;*.midi");
            chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this] (const FileChooser& fc)
            {
                auto file = fc.getResult();
                if (! file.existsAsFile()) return;
                FileInputStream in (file);
                MidiFile midi;
                Pattern imported;
                if (in.openedOk() && midi.readFrom (in) && midiToPattern (midi, roundToInt (value (ParamIDs::seqRate)), imported))
                    processor.editPattern (processor.getCurrentSlot(), [imported] (Pattern& pat) { pat = imported; });
            });
        }
    });
}

//==============================================================================
void SynthPage::paint (Graphics& g)
{
    AcidLookAndFeel::drawPanel (g, { 8.0f, 0.0f, 150.0f, 206.0f }, "Osc");
    AcidLookAndFeel::drawPanel (g, { 164.0f, 0.0f, 420.0f, 206.0f }, "Filter");
    AcidLookAndFeel::drawPanel (g, { 590.0f, 0.0f, 402.0f, 206.0f }, "Envelope");
    AcidLookAndFeel::drawPanel (g, { 8.0f, 212.0f, 984.0f, 378.0f }, "Sequencer");

    AcidLookAndFeel::drawSmallLabel (g, "Wave", { 20.0f, 22.0f, 126.0f, 12.0f }, Justification::centredLeft);
    AcidLookAndFeel::drawSmallLabel (g, "Rate", { 566.0f, 236.0f, 84.0f, 12.0f }, Justification::centredLeft);
    AcidLookAndFeel::drawSmallLabel (g, "Direction", { 660.0f, 236.0f, 110.0f, 12.0f }, Justification::centredLeft);
    AcidLookAndFeel::drawSmallLabel (g, "Steps", { 784.0f, 236.0f, 196.0f, 12.0f }, Justification::centredLeft);

    // separator between the controls and the grid
    g.setColour (Theme::get().bevelDark);
    g.drawHorizontalLine (296, 20.0f, 980.0f);
    g.setColour (Colours::white.withAlpha (0.07f));
    g.drawHorizontalLine (297, 20.0f, 980.0f);
}

void SynthPage::resized()
{
    // OSC
    waveButton.setBounds (20, 36, 126, 22);
    waveDisplay.setBounds (20, 64, 126, 82);
    retroButton.setBounds (20, 156, 126, 22);

    // FILTER
    cutoffKnob.setBounds (178, 24, 116, 116);
    resoKnob.setBounds (304, 24, 116, 116);
    cutoffLcd.setBounds (178, 148, 116, 22);
    resoLcd.setBounds (304, 148, 116, 22);
    cutoffLowKnob.setBounds (430, 22, 72, 76);
    driveKnob.setBounds (504, 22, 72, 76);
    bassKnob.setBounds (430, 100, 72, 76);
    trebleKnob.setBounds (504, 100, 72, 76);
    transistorButton.setBounds (178, 176, 242, 22);

    // ENVELOPE
    const int ex = 600, step = 96;
    envModKnob.setBounds (ex, 22, 84, 84);
    decayKnob.setBounds (ex + step, 22, 84, 84);
    accentKnob.setBounds (ex + step * 2, 22, 84, 84);
    gateKnob.setBounds (ex + step * 3, 22, 84, 84);
    envAttackKnob.setBounds (ex, 112, 84, 84);
    accDecayKnob.setBounds (ex + step, 112, 84, 84);
    accSweepKnob.setBounds (ex + step * 2, 112, 84, 84);
    envMeter.setBounds (ex + step * 3 - 4, 124, 92, 58);

    // SEQUENCER controls
    seqLcd.setBounds (20, 236, 200, 24);
    playButton.setBounds (20, 266, 62, 24);
    seqButton.setBounds (88, 266, 62, 24);
    keyButton.setBounds (156, 266, 64, 24);
    patternKnob.setBounds (232, 230, 62, 64);
    lengthKnob.setBounds (298, 230, 62, 64);
    swingKnob.setBounds (364, 230, 62, 64);
    slideKnob.setBounds (430, 230, 62, 64);
    transposeKnob.setBounds (496, 230, 62, 64);
    rateBox.setBounds (566, 252, 84, 22);
    directionBox.setBounds (660, 252, 110, 22);
    for (int i = 0; i < 4; ++i)
        pageButtons[(size_t) i].setBounds (784 + i * 49, 252, 46, 22);

    // tools row
    int x = 20;
    auto place = [&x] (Component& c, int w) { c.setBounds (x, 304, w, 22); x += w + 5; };
    place (randomButton, 88);
    place (mutateButton, 70);
    place (clearButton, 60);
    place (rotateLeftButton, 28);
    place (rotateRightButton, 28);
    place (copyButton, 56);
    place (pasteButton, 56);
    place (undoButton, 56);
    place (redoButton, 56);
    place (midiButton, 70);
    scaleLcd.setBounds (x + 6, 304, 150, 22);
    followButton.setBounds (904, 304, 76, 22);

    grid.setBounds (20, 332, 960, 250);
}

//==============================================================================
void SynthPage::update()
{
    auto& gui = processor.gui;

    // OSC / filter readouts
    const bool square = value (ParamIDs::wave) > 0.5f;
    waveButton.setButtonText (square ? "SQUARE" : "SAW");
    waveDisplay.setMorph (square ? 1.0f : 0.0f);
    cutoffLcd.setContent ("CUTOFF", formatHz (cutoffToHz (value (ParamIDs::cutoff))));
    resoLcd.setContent ("RESONANCE", formatNumber (value (ParamIDs::resonance)));
    envMeter.setLevels (gui.filterEnv.load(), jlimit (0.0f, 1.0f, gui.accentCharge.load()));

    // modulation arcs where an LFO targets a knob
    float cutoffMod = 0.0f, resoMod = 0.0f;
    for (int i = 0; i < 2; ++i)
    {
        const float depth = value (ParamIDs::lfo (i, "Depth")) / 100.0f;
        const int dest = roundToInt (value (ParamIDs::lfo (i, "Dest")));
        if (dest == DestCutoff) cutoffMod += std::abs (depth) * 0.4f;   // 4 of the 10 octaves
        if (dest == DestReso)   resoMod += std::abs (depth);
    }
    cutoffKnob.setModDepth (jmin (1.0f, cutoffMod));
    resoKnob.setModDepth (jmin (1.0f, resoMod));

    // transport
    const bool running = gui.running.load();
    const bool host = gui.hostPlaying.load();
    playButton.setToggleState (processor.isInternalPlayOn(), dontSendNotification);
    playButton.setButtonText (processor.isInternalPlayOn() ? "STOP" : "PLAY");

    const int slot = processor.getCurrentSlot();
    const auto& pat = processor.getPattern (slot);
    String state = running ? (host ? "HOST" : "PLAYING") : "STOPPED";
    seqLcd.setContent (String (gui.bpm.load(), 1) + " BPM   " + state,
                       "P" + String (slot + 1).paddedLeft ('0', 2) + "  " + String (pat.length) + " ST");

    scaleLcd.setContent (PatternTools::rootName (processor.randomRoot),
                         PatternTools::scales()[(size_t) processor.randomScale].name);

    // pattern changes
    const int version = processor.patternVersion.load();
    if (version != lastPatternVersion)
    {
        lastPatternVersion = version;
        if (! lengthDragging)
            lengthKnob.setValue (pat.length, dontSendNotification);
        grid.repaint();
    }

    const int stepNow = gui.currentStep.load();
    grid.setPlayStep (stepNow);
    if (followButton.getToggleState() && stepNow >= 0 && stepNow / 16 != grid.getPage())
        setGridPage (stepNow / 16);

    undoButton.setEnabled (processor.canUndoPattern());
    redoButton.setEnabled (processor.canRedoPattern());
    pasteButton.setEnabled (processor.hasClipboard());
}
