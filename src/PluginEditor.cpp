#include "PluginEditor.h"
#include "presets/PresetManager.h"
#include "ui/arp/ArpCompat.h"
#include <map>

using namespace juce;
using namespace acid;

namespace
{
    constexpr int kW = AcidPanel::kBaseWidth, kH = AcidPanel::kBaseHeight;
    constexpr int kHeaderH = 58, kTabsY = 64, kTabsH = 26, kContentY = 96, kBottomBarY = 870;
    const char* kPageNames[] = { "Synth / Seq", "Layers", "Tube" };
    constexpr int kNumPages = 3;
}

//==============================================================================
AcidPanel::AcidPanel (AcidAudioProcessor& p)
    : processor (p), synthPage (p), layersPage (p), tubePage (p)
{
    Theme::setLight (processor.gui.lightTheme.load());
    VirusColours::refresh();
    lnf.refreshColours();
    setLookAndFeel (&lnf);
    setOpaque (true);
    setSize (kW, kH);
    createBackground();

    // header ------------------------------------------------------------------------
    saveButton.onClick = [this] { savePreset(); };
    setHelp (saveButton, "SAVE  -  save the sound and its 16 patterns as a user preset");
    clearButton.onClick = [this] { processor.initSound(); };
    setHelp (clearButton, "CLEAR  -  every sound setting back to default (the patterns stay)");
    presetPrev.onClick = [this] { processor.presets.step (-1); };
    setHelp (presetPrev, "PREVIOUS PRESET");
    presetNext.onClick = [this] { processor.presets.step (1); };
    setHelp (presetNext, "NEXT PRESET");
    display.onClick = [this] { showPresetMenu(); };
    setHelp (display, "PRESET  -  click to browse factory and user presets");

    for (auto* b : { &saveButton, &clearButton, &presetPrev, &presetNext })
        addAndMakeVisible (*b);
    addAndMakeVisible (display);

    const std::pair<TextButton*, const char*> bypassButtons[] = { { &echoButton, ParamIDs::echoOn }, { &tubeButton, ParamIDs::tubeOn } };
    for (auto [button, id] : bypassButtons)
    {
        button->setClickingTogglesState (true);
        buttonAttachments.push_back (std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, id, *button));
        setHelp (*button, button->getButtonText().toUpperCase() + "  -  " + helpForParameter (id));
        addAndMakeVisible (*button);
    }

    themeButton.onClick = [this]
    {
        processor.gui.lightTheme.store (! processor.gui.lightTheme.load());
        applyTheme();
    };
    setHelp (themeButton, "THEME  -  switch between the dark and the light colours");
    addAndMakeVisible (themeButton);

    const std::pair<AcidKnob*, const char*> headerKnobs[] = { { &tuneKnob, ParamIDs::tune }, { &volumeKnob, ParamIDs::volume } };
    for (auto [knob, id] : headerKnobs)
    {
        knobAttachments.push_back (std::make_unique<KnobAttachment> (*processor.apvts.getParameter (id), *knob));
        addAndMakeVisible (*knob);
    }

    // tabs + pages -------------------------------------------------------------------
    for (int i = 0; i < kNumPages; ++i)
    {
        auto& tab = tabs[(size_t) i];
        tab.setButtonText (kPageNames[i]);
        tab.onClick = [this, i] { showPage (i); };
        setHelp (tab, String (kPageNames[i]).toUpperCase() + " PAGE");
        addAndMakeVisible (tab);
    }
    addChildComponent (synthPage);
    addChildComponent (layersPage);
    addChildComponent (tubePage);

    helpButton.onClick = [this] { helpOverlay.setVisible (true); helpOverlay.toFront (false); };
    setHelp (helpButton, "HELP  -  quick reference");
    addAndMakeVisible (helpButton);
    addChildComponent (helpOverlay);

    // hover help: listen to the mouse over every child
    addMouseListener (this, true);

    showPage (jlimit (0, kNumPages - 1, processor.gui.page.load()));
    timerCallback();
    startTimerHz (30);
}

AcidPanel::~AcidPanel()
{
    stopTimer();
    removeMouseListener (this);
    setLookAndFeel (nullptr);
}

//==============================================================================
void AcidPanel::showPage (int page)
{
    currentPage = page;
    processor.gui.page.store (page);
    synthPage.setVisible (page == 0);
    layersPage.setVisible (page == 1);
    tubePage.setVisible (page == 2);
    for (int i = 0; i < kNumPages; ++i)
        tabs[(size_t) i].setToggleState (i == page, dontSendNotification);
    repaint();
}

void AcidPanel::applyTheme()
{
    const bool light = processor.gui.lightTheme.load();
    Theme::setLight (light);
    VirusColours::refresh();
    lightShown = light;
    lnf.refreshColours();
    createBackground();
    themeButton.setButtonText (light ? "LIGHT" : "DARK");
    sendLookAndFeelChange();
    repaint();
}

//==============================================================================
void AcidPanel::resized()
{
    saveButton.setBounds (180, 8, 58, 20);
    clearButton.setBounds (180, 31, 58, 20);
    presetPrev.setBounds (246, 6, 20, 46);
    display.setBounds (268, 6, 336, 46);
    presetNext.setBounds (606, 6, 20, 46);
    echoButton.setBounds (636, 8, 80, 20);
    tubeButton.setBounds (636, 31, 80, 20);
    themeButton.setBounds (724, 8, 70, 20);
    tuneKnob.setBounds (852, 2, 64, 54);
    volumeKnob.setBounds (924, 2, 64, 54);

    for (int i = 0; i < kNumPages; ++i)
        tabs[(size_t) i].setBounds (8 + i * 124, kTabsY, 120, kTabsH);

    const Rectangle<int> content (0, kContentY, kW, kBottomBarY - kContentY - 4);
    synthPage.setBounds (content);
    layersPage.setBounds (content);
    tubePage.setBounds (content);

    helpButton.setBounds (kW - 28, kBottomBarY + 5, 20, 20);
    helpOverlay.setBounds (getLocalBounds());
}

//==============================================================================
void AcidPanel::createBackground()
{
    const auto& t = Theme::get();
    background = Image (Image::RGB, kW * 2, kH * 2, true);
    Graphics g (background);
    g.addTransform (AffineTransform::scale (2.0f));

    g.setGradientFill (ColourGradient (t.background1, 0.0f, 0.0f, t.background2, 0.0f, (float) kH, false));
    g.fillAll();

    // brushed metal: faint full-width lines plus random short streaks
    Random rng (0x7a11);
    const float strength = Theme::isLight() ? 0.5f : 1.0f;
    for (int y = 0; y < kH; ++y)
    {
        g.setColour ((rng.nextBool() ? Colours::white : Colours::black).withAlpha (rng.nextFloat() * 0.035f * strength));
        g.fillRect (0, y, kW, 1);
    }
    for (int i = 0; i < 2400; ++i)
    {
        const int x = rng.nextInt (kW), y = rng.nextInt (kH), len = 20 + rng.nextInt (160);
        g.setColour ((rng.nextBool() ? Colours::white : Colours::black).withAlpha ((0.015f + rng.nextFloat() * 0.03f) * strength));
        g.fillRect (x, y, len, 1);
    }

    ColourGradient vignette (Colours::transparentBlack, kW * 0.5f, kH * 0.45f,
                             Colours::black.withAlpha (Theme::isLight() ? 0.12f : 0.35f), 0.0f, 0.0f, true);
    g.setGradientFill (vignette);
    g.fillAll();
}

void AcidPanel::drawWordmark (Graphics& g, Rectangle<float> area)
{
    // original lowercase wordmark: chrome "acid" with an orange underline swoosh
    const float baseline = area.getBottom() - 12.0f;

    GlyphArrangement main;
    main.addLineOfText (Font (FontOptions (34.0f, Font::bold | Font::italic)), "acid", area.getX(), baseline);
    Path mainPath;
    main.createPath (mainPath);
    const auto mb = mainPath.getBounds();

    g.setColour (Colours::black.withAlpha (0.6f));
    g.fillPath (mainPath, AffineTransform::translation (1.0f, 1.5f));
    ColourGradient chrome (Colour (0xffffffff), 0.0f, mb.getY(), Colour (0xff8c949c), 0.0f, mb.getBottom(), false);
    chrome.addColour (0.5, Colour (0xffd5dade));
    chrome.addColour (0.55, Colour (0xffa4acb3));
    if (Theme::isLight())
        chrome = ColourGradient (Colour (0xff5a5f66), 0.0f, mb.getY(), Colour (0xff1c1e22), 0.0f, mb.getBottom(), false);
    g.setGradientFill (chrome);
    g.fillPath (mainPath);

    // small orange tag
    g.setFont (AcidLookAndFeel::uiFont (10.0f, true));
    g.setColour (Theme::get().accent);
    g.drawText ("BASS SYNTH", Rectangle<float> (mb.getRight() + 8.0f, mb.getBottom() - 12.0f, 80.0f, 12.0f), Justification::centredLeft);

    Path swoosh;
    const float sx = mb.getX() + 2.0f, sy = baseline + 5.0f, ex = mb.getRight() + 70.0f;
    swoosh.startNewSubPath (sx, sy);
    swoosh.quadraticTo ((sx + ex) * 0.5f, sy + 3.5f, ex, sy - 2.0f);
    g.setGradientFill (ColourGradient (Theme::get().accent.withAlpha (0.0f), sx, sy, Theme::get().accent, ex, sy, false));
    g.strokePath (swoosh, PathStrokeType (1.6f, PathStrokeType::curved, PathStrokeType::rounded));
}

void AcidPanel::paint (Graphics& g)
{
    const auto& t = Theme::get();
    g.drawImage (background, getLocalBounds().toFloat());

    // header strip
    auto top = Rectangle<float> (0.0f, 0.0f, (float) kW, (float) kHeaderH);
    g.setGradientFill (ColourGradient (t.barTop, 0.0f, 0.0f, t.barBottom, 0.0f, top.getBottom(), false));
    g.fillRect (top);
    g.setColour (Colours::white.withAlpha (0.18f));
    g.drawHorizontalLine (0, 0.0f, (float) kW);
    g.setColour (t.bevelDark);
    g.drawHorizontalLine (kHeaderH - 1, 0.0f, (float) kW);
    g.setColour (Colours::white.withAlpha (0.06f));
    g.drawHorizontalLine (kHeaderH, 0.0f, (float) kW);

    drawWordmark (g, { 16.0f, 2.0f, 160.0f, (float) kHeaderH - 2.0f });

    // tabs groove, with a small status readout at the right
    AcidLookAndFeel::drawInsetBox (g, { 5.0f, (float) kTabsY - 3.0f, (float) kW - 10.0f, (float) kTabsH + 6.0f }, 4.0f);
    {
        auto status = Rectangle<float> (650.0f, (float) kTabsY, 340.0f, (float) kTabsH);
        const bool running = processor.gui.running.load();
        g.setFont (AcidLookAndFeel::uiFont (11.0f, true));
        g.setColour (running ? t.accent : t.labelText);
        const int step = processor.gui.currentStep.load();
        g.drawText (String (processor.gui.bpm.load(), 1) + (processor.gui.hostTempo.load() ? " BPM HOST" : " BPM INT")
                        + (running && step >= 0 ? "    STEP " + String (step + 1) : String ("    STOPPED")),
                    status.reduced (8.0f, 0.0f), Justification::centredRight);
        // gate LED
        const bool gate = processor.gui.gateOn.load();
        auto led = Rectangle<float> (10.0f, 10.0f).withCentre ({ status.getX() + 4.0f, status.getCentreY() });
        g.setColour (gate ? t.accent : t.labelText.withAlpha (0.3f));
        g.fillEllipse (led);
        if (gate)
        {
            g.setColour (t.accent.withAlpha (0.3f));
            g.fillEllipse (led.expanded (4.0f));
        }
    }

    // bottom bar
    auto bar = Rectangle<float> (0.0f, (float) kBottomBarY, (float) kW, (float) (kH - kBottomBarY));
    g.setGradientFill (ColourGradient (t.barBottom, 0.0f, bar.getY(), t.background2, 0.0f, bar.getBottom(), false));
    g.fillRect (bar);
    g.setColour (Colours::white.withAlpha (0.1f));
    g.drawHorizontalLine (kBottomBarY, 0.0f, (float) kW);

    auto tab = Rectangle<float> (8.0f, bar.getY() + 6.0f, 66.0f, 18.0f);
    g.setGradientFill (ColourGradient (t.headerTop, 0.0f, tab.getY(), t.headerBottom, 0.0f, tab.getBottom(), false));
    g.fillRoundedRectangle (tab, 3.0f);
    g.setColour (t.bevelDark);
    g.drawRoundedRectangle (tab, 3.0f, 1.0f);
    g.setColour (t.headerText);
    g.setFont (AcidLookAndFeel::uiFont (10.5f, true));
    g.drawText (String (kPageNames[currentPage]).toUpperCase(), tab, Justification::centred);

    auto inset = bar.reduced (90.0f, 6.0f);
    AcidLookAndFeel::drawInsetBox (g, inset, 3.0f);
    g.setFont (AcidLookAndFeel::uiFont (13.0f, true));
    g.setColour (Theme::isLight() ? t.headerText : t.lightGrey);
    g.drawText (String (ACID_PRODUCT_NAME).toUpperCase(), inset, Justification::centred);
}

//==============================================================================
// Hover help
//==============================================================================
String AcidPanel::helpFor (Component* c) const
{
    for (auto* comp = c; comp != nullptr && comp != this; comp = comp->getParentComponent())
    {
        if (auto* knob = dynamic_cast<AcidKnob*> (comp))      return knob->getHelpLine();
        if (auto* grid = dynamic_cast<ArpStepGrid*> (comp))   return grid->getHoverHelp();
        if (auto* graph = dynamic_cast<BandGraph*> (comp))    return graph->getHoverHelp();
        if (comp->getProperties().contains ("help"))          return comp->getProperties()["help"].toString();
        if (auto* tip = dynamic_cast<TooltipClient*> (comp))
            if (auto text = tip->getTooltip(); text.isNotEmpty())
                return text.replace ("\n", "  ");
    }
    return {};
}

void AcidPanel::trackHover (Component* c)
{
    hovered = c;
    display.setHelpText (helpFor (c));
}

void AcidPanel::mouseMove (const MouseEvent& e)   { trackHover (e.eventComponent); }
void AcidPanel::mouseEnter (const MouseEvent& e)  { trackHover (e.eventComponent); }
void AcidPanel::mouseDrag (const MouseEvent& e)   { trackHover (e.eventComponent); }
void AcidPanel::mouseExit (const MouseEvent&)     { }

//==============================================================================
// Presets
//==============================================================================
void AcidPanel::showPresetMenu()
{
    const auto entries = processor.presets.list();
    PopupMenu m;
    std::map<String, PopupMenu> banks;
    StringArray bankOrder;
    for (int i = 0; i < (int) entries.size(); ++i)
    {
        const auto& e = entries[(size_t) i];
        if (! e.factory) continue;
        if (! bankOrder.contains (e.bank)) bankOrder.add (e.bank);
        banks[e.bank].addItem (i + 1, e.name, true, e.name == processor.presets.getCurrentName());
    }
    m.addSectionHeader ("Factory");
    for (auto& b : bankOrder)
        m.addSubMenu (b, banks[b]);

    m.addSectionHeader ("User");
    bool anyUser = false;
    for (int i = 0; i < (int) entries.size(); ++i)
        if (! entries[(size_t) i].factory)
        {
            m.addItem (i + 1, entries[(size_t) i].name, true, entries[(size_t) i].name == processor.presets.getCurrentName());
            anyUser = true;
        }
    if (! anyUser)
        m.addItem (-1, "(none yet: use Save)", false);

    m.addSeparator();
    m.addItem (10000, "Save as...");
    m.addItem (10001, "Load preset file...");
    m.addItem (10002, "Open user preset folder");

    m.showMenuAsync (PopupMenu::Options().withTargetComponent (&display), [this, entries] (int r)
    {
        if (r >= 1 && r <= (int) entries.size())
            processor.presets.loadEntry (entries[(size_t) r - 1]);
        else if (r == 10000)
            savePreset();
        else if (r == 10001)
        {
            chooser = std::make_unique<FileChooser> ("Load preset", PresetManager::userFolder(), "*.json");
            chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this] (const FileChooser& fc)
            {
                if (fc.getResult().existsAsFile())
                    processor.presets.loadFile (fc.getResult());
            });
        }
        else if (r == 10002)
        {
            PresetManager::userFolder().createDirectory();
            PresetManager::userFolder().startAsProcess();
        }
    });
}

void AcidPanel::savePreset()
{
    auto folder = PresetManager::userFolder();
    folder.createDirectory();
    chooser = std::make_unique<FileChooser> ("Save preset", folder.getChildFile (File::createLegalFileName (processor.presets.getCurrentName()) + ".json"), "*.json");
    chooser->launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::warnAboutOverwriting, [this] (const FileChooser& fc)
    {
        auto file = fc.getResult();
        if (file == File()) return;
        file = file.withFileExtension ("json");
        processor.presets.saveFile (file, file.getFileNameWithoutExtension());
    });
}

//==============================================================================
void AcidPanel::timerCallback()
{
    if (processor.gui.lightTheme.load() != lightShown || lastPresetVersion < 0)
        applyTheme();

    const int pv = processor.presets.version.load();
    if (pv != lastPresetVersion)
    {
        lastPresetVersion = pv;
        display.setPreset (processor.presets.getCurrentBank(), processor.presets.getCurrentName());
    }

    // keep the help text live while a value changes under the mouse
    if (hovered != nullptr)
        display.setHelpText (helpFor (hovered.getComponent()));

    switch (currentPage)
    {
        case 0:  synthPage.update(); break;
        case 1:  layersPage.update(); break;
        default: tubePage.update(); break;
    }

    repaint (640, kTabsY, 360, kTabsH);   // status readout
}

//==============================================================================
// Resizable editor: the panel is laid out at its base size and scaled as a whole.
//==============================================================================
AcidAudioProcessorEditor::AcidAudioProcessorEditor (AcidAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p), panel (p)
{
    // Read the remembered size first: setting the limits resizes the window briefly.
    const int savedWidth = processor.gui.editorWidth.load();

    addAndMakeVisible (panel);
    setResizable (true, true);
    setResizeLimits (kW * 6 / 10, kH * 6 / 10, kW * 2, kH * 2);
    getConstrainer()->setFixedAspectRatio ((double) kW / (double) kH);

    const int width = savedWidth > 0 ? jlimit (kW * 6 / 10, kW * 2, savedWidth) : kW;
    setSize (width, roundToInt (width * (double) kH / (double) kW));
    sizeRestored = true;
}

void AcidAudioProcessorEditor::resized()
{
    panel.setBounds (0, 0, kW, kH);
    panel.setTransform (AffineTransform::scale ((float) getWidth() / (float) kW));

    if (sizeRestored)
        processor.gui.editorWidth.store (getWidth());
}
