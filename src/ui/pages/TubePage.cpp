#include "TubePage.h"

using namespace juce;
using namespace acid;


TubePage::TubePage (AcidAudioProcessor& p)
    : PageBase (p)
{
    using namespace ParamIDs;

    // Saturate / Foldback
    attach (distOnButton, distOn);
    setHelp (distOnButton, "ON  -  " + helpForParameter (distOn));
    const auto names = distModeNames();
    for (int i = 0; i < names.size(); ++i)
    {
        auto* b = modeButtons.add (new TextButton (names[i]));
        attach (*b, distModeIds[i]);
        setHelp (*b, names[i] + "  -  " + helpForParameter (distModeIds[i]));
    }
    attach (distDrive, ParamIDs::distDrive);
    attach (distAmount, ParamIDs::distAmount);

    // the tube (one band)
    attach (tubeOnButton, tubeOn);
    setHelp (tubeOnButton, "ON  -  " + helpForParameter (tubeOn));
    generateButton.onClick = [this] { processor.generateTube(); };
    setHelp (generateButton, "GENERATE DISTORTION  -  random tube settings in musical ranges (Undo takes it back)");
    undoButton.onClick = [this] { processor.undoTube(); };
    setHelp (undoButton, "UNDO  -  back to the tube settings before Generate / Init");
    initButton.onClick = [this] { processor.initTube(); };
    setHelp (initButton, "INIT  -  tube settings back to their defaults");
    redoButton.onClick = [this] { processor.redoTube(); };
    setHelp (redoButton, "REDO  -  redo a tube Generate / Init");
    for (auto* b : { &generateButton, &undoButton, &initButton, &redoButton })
        addAndMakeVisible (*b);

    attach (resonator, tubeResonator);
    attach (character, tubeCharacter);
    attach (input, tubeInput);
    attach (output, tubeOutput);
    attach (mix, tubeMix);

    attach (tubeDrive, ParamIDs::tubeDrive);
    addAndMakeVisible (tube);
    setHelp (tube, "TUBE  -  glows with the drive and the level");
    addAndMakeVisible (tubeDriveLcd);
    setHelp (tubeDriveLcd, "TUBE DRIVE");


    // Delay
    attach (delayOn, ParamIDs::echoOn);
    attach (delaySync, ParamIDs::echoSync);
    attach (delayTime, ParamIDs::echoTime);
    attach (delayMix, ParamIDs::echoMix);
    attach (delayFeedback, ParamIDs::echoFeedback);
    attach (delayLowCut, ParamIDs::echoLowCut);
    attach (delayHighCut, ParamIDs::echoHighCut);
    attach (delayWidth, ParamIDs::echoWidth);
    addAndMakeVisible (delayLcd);
    setHelp (delayLcd, "DELAY  -  the current delay time");
    delayTime.displayValue = [this] (double v)
    {
        if (value (ParamIDs::echoSync) > 0.5f)
            return String (echoDivisions()[(size_t) divisionIndex ((float) v, (int) echoDivisions().size())].name);
        return formatMs (echoFreeMs ((float) v));
    };
}

void TubePage::paint (Graphics& g)
{
    // row 1: Distortion | Delay; row 2: Tube
    AcidLookAndFeel::drawPanel (g, { 8.0f, 0.0f, 520.0f, 117.0f }, "Distortion");
    AcidLookAndFeel::drawPanel (g, { 534.0f, 0.0f, 458.0f, 117.0f }, "Delay");
    AcidLookAndFeel::drawPanel (g, { 8.0f, 123.0f, 984.0f, 117.0f }, "Tube");
}

void TubePage::resized()
{
    // Distortion: ON, the six modes (any number on), Drive and Distort
    distOnButton.setBounds (16, 30, 60, 22);
    for (int i = 0; i < modeButtons.size(); ++i)
        modeButtons[i]->setBounds (86 + (i % 3) * 84, 34 + (i / 3) * 30, 82, 26);
    distDrive.setBounds (352, 24, 76, 88);
    distAmount.setBounds (436, 24, 76, 88);

    // Delay: ON over Sync, then six knobs
    delayOn.setBounds (542, 34, 56, 22);
    delaySync.setBounds (542, 66, 56, 22);
    int x = 604;
    for (auto* k : { &delayTime, &delayMix, &delayFeedback, &delayLowCut, &delayHighCut, &delayWidth })
    {
        k->setBounds (x, 24, 62, 88);
        x += 64;
    }
    delayLcd.setVisible (false);

    // Tube: ON / Generate / Undo Init Redo, the five knobs, the big Drive and the tube
    const int ty = 123;
    tubeOnButton.setBounds (16, ty + 30, 60, 22);
    generateButton.setBounds (84, ty + 30, 168, 22);
    undoButton.setBounds (84, ty + 62, 54, 22);
    initButton.setBounds (141, ty + 62, 54, 22);
    redoButton.setBounds (198, ty + 62, 54, 22);
    x = 280;
    for (auto* k : { &resonator, &character, &input, &output, &mix })
    {
        k->setBounds (x, ty + 24, 70, 88);
        x += 84;
    }
    tubeDrive.setBounds (x + 20, ty + 20, 100, 94);
    tubeDriveLcd.setVisible (false);
    tube.setBounds (x + 150, ty + 22, 56, 90);
}

void TubePage::update()
{
    const float t = value (ParamIDs::echoTime);
    if (value (ParamIDs::echoOn) < 0.5f)
        delayLcd.setContent ("DELAY OFF", "tail rings out");
    else if (value (ParamIDs::echoSync) > 0.5f)
    {
        const auto& d = echoDivisions()[(size_t) divisionIndex (t, (int) echoDivisions().size())];
        delayLcd.setContent ("TIME " + String (d.name), formatMs ((float) (d.beats * 60000.0 / jmax (20.0f, processor.gui.bpm.load()))));
    }
    else
        delayLcd.setContent ("TIME", formatMs (echoFreeMs (t)));


    const float drive = value (ParamIDs::tubeDrive);
    const bool active = value (ParamIDs::tubeOn) > 0.5f && value (ParamIDs::distPageOn) > 0.5f;
    float level = 0.0f;
    for (int b = 0; b < 3; ++b) level = jmax (level, processor.gui.bandLevel[b].load());
    tube.setGlow (active ? drive / 100.0f : 0.0f, active ? level : 0.0f);
    tubeDriveLcd.setContent ("DRIVE", formatNumber (drive));

    undoButton.setEnabled (processor.canUndoTube());
    redoButton.setEnabled (processor.canRedoTube());
}
