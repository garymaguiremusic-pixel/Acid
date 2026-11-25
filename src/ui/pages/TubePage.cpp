#include "TubePage.h"

using namespace juce;
using namespace acid;

namespace { const char* kBandTitles[] = { "Low", "Mid", "High" }; }

TubePage::TubePage (AcidAudioProcessor& p) : PageBase (p), graph (p)
{
    using namespace ParamIDs;
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

    for (int b = 0; b < 3; ++b)
    {
        auto& c = bands[b];
        attach (c.drive, band (b, "Drive"));
        attach (c.gain, band (b, "Gain"));
        attach (c.solo, band (b, "Solo"));
        addAndMakeVisible (c.tube);
        setHelp (c.tube, String (kBandTitles[b]).toUpperCase() + " TUBE  -  glows with the band's drive and level");
        addAndMakeVisible (c.driveLcd);
        setHelp (c.driveLcd, String (kBandTitles[b]).toUpperCase() + " DRIVE");
    }
    addAndMakeVisible (graph);
}

void TubePage::paint (Graphics& g)
{
    AcidLookAndFeel::drawPanel (g, { 8.0f, 0.0f, 984.0f, 120.0f }, "Tube Distortion  -  three bands");
    const float edges[4] = { value (ParamIDs::tubeEdgeLow), value (ParamIDs::tubeXover1),
                             value (ParamIDs::tubeXover2), value (ParamIDs::tubeEdgeHigh) };
    for (int b = 0; b < 3; ++b)
    {
        const float x0 = 8.0f + 330.0f * (float) b;
        AcidLookAndFeel::drawPanel (g, { x0, 126.0f, 324.0f, 230.0f }, kBandTitles[b]);
        g.setFont (AcidLookAndFeel::uiFont (11.0f, true));
        g.setColour (Theme::get().labelText);
        g.drawText (formatHz (edges[b]) + "  -  " + formatHz (edges[b + 1]), Rectangle<float> (x0 + 12.0f, 148.0f, 180.0f, 14.0f), Justification::centredLeft);
    }
    AcidLookAndFeel::drawPanel (g, { 8.0f, 362.0f, 984.0f, (float) getHeight() - 362.0f }, "Bands");
}

void TubePage::resized()
{
    generateButton.setBounds (24, 30, 232, 32);
    undoButton.setBounds (24, 72, 74, 24);
    initButton.setBounds (103, 72, 74, 24);
    redoButton.setBounds (182, 72, 74, 24);

    int x = 330;
    for (auto* k : { &resonator, &character, &input, &output, &mix })
    {
        k->setBounds (x, 24, 90, 88);
        x += 128;
    }

    for (int b = 0; b < 3; ++b)
    {
        const int x0 = 8 + 330 * b;
        auto& c = bands[b];
        c.drive.setBounds (x0 + 14, 170, 112, 112);
        c.driveLcd.setBounds (x0 + 14, 290, 112, 22);
        c.gain.setBounds (x0 + 132, 188, 66, 78);
        c.solo.setBounds (x0 + 14, 320, 184, 24);
        c.tube.setBounds (x0 + 206, 148, 108, 198);
    }

    graph.setBounds (16, 384, 968, getHeight() - 392);
}

void TubePage::update()
{
    for (int b = 0; b < 3; ++b)
    {
        const float drive = value (ParamIDs::band (b, "Drive"));
        const bool active = value (ParamIDs::tubeOn) > 0.5f;
        bands[b].tube.setGlow (active ? drive / 100.0f : 0.0f, active ? processor.gui.bandLevel[b].load() : 0.0f);
        bands[b].driveLcd.setContent ("DRIVE", formatNumber (drive));
    }

    // repaint the graph and the band ranges only when they change
    float state[7];
    state[0] = value (ParamIDs::tubeEdgeLow); state[1] = value (ParamIDs::tubeXover1);
    state[2] = value (ParamIDs::tubeXover2);  state[3] = value (ParamIDs::tubeEdgeHigh);
    for (int b = 0; b < 3; ++b) state[4 + b] = value (ParamIDs::band (b, "Gain"));
    if (! std::equal (std::begin (state), std::end (state), std::begin (lastGraphState)))
    {
        std::copy (std::begin (state), std::end (state), std::begin (lastGraphState));
        graph.repaint();
        repaint (0, 140, getWidth(), 30);
    }

    undoButton.setEnabled (processor.canUndoTube());
    redoButton.setEnabled (processor.canRedoTube());
}
