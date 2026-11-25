#include "LayersPage.h"

using namespace juce;
using namespace acid;

LayersPage::LayersPage (AcidAudioProcessor& p) : PageBase (p)
{
    using namespace ParamIDs;
    attach (oscBLevel, ParamIDs::oscBLevel);
    attach (oscBTune, ParamIDs::oscBTune);
    attach (oscBDetune, ParamIDs::oscBDetune);
    attach (oscBLockButton, ParamIDs::oscBLock);
    attach (unisonFree, ParamIDs::unisonFree);
    attach (voices, unisonVoices);
    attach (spread, unisonSpread);
    attach (ampAttack, ParamIDs::ampAttack);
    attach (ampDecay, ParamIDs::ampDecay);
    attach (ampRelease, ParamIDs::ampRelease);
    addAndMakeVisible (ampLcd);
    setHelp (ampLcd, "AMP ENVELOPE  -  attack / decay / release of the VCA");

    attach (echoSync, ParamIDs::echoSync);
    attach (echoTime, ParamIDs::echoTime);
    attach (echoMix, ParamIDs::echoMix);
    attach (echoFeedback, ParamIDs::echoFeedback);
    attach (echoLowCut, ParamIDs::echoLowCut);
    attach (echoHighCut, ParamIDs::echoHighCut);
    attach (echoWidth, ParamIDs::echoWidth);
    addAndMakeVisible (echoLcd);
    setHelp (echoLcd, "ECHO  -  the current echo time");

    // echo time and LFO rates show only the active unit
    echoTime.displayValue = [this] (double v)
    {
        if (value (ParamIDs::echoSync) > 0.5f)
            return String (echoDivisions()[(size_t) divisionIndex ((float) v, (int) echoDivisions().size())].name);
        return formatMs (echoFreeMs ((float) v));
    };

    setupLfo (0);
    setupLfo (1);
}

void LayersPage::setupLfo (int i)
{
    auto& c = lfo[i];
    attach (c.rate, ParamIDs::lfo (i, "Rate"));
    attach (c.depth, ParamIDs::lfo (i, "Depth"));
    attach (c.phase, ParamIDs::lfo (i, "Phase"));
    attach (c.sync, ParamIDs::lfo (i, "Sync"));
    attach (c.mirror, ParamIDs::lfo (i, "Mirror"));
    attach (c.retrig, ParamIDs::lfo (i, "Retrig"));
    addAndMakeVisible (c.preview);
    setHelp (c.preview, "LFO " + String (i + 1) + " SHAPE  -  " + helpForParameter (ParamIDs::lfo (i, "Shape")));

    c.rate.displayValue = [this, i] (double v)
    {
        if (value (ParamIDs::lfo (i, "Sync")) > 0.5f)
            return String (lfoDivisions()[(size_t) divisionIndex ((float) v, (int) lfoDivisions().size())].name);
        return formatNumber (lfoFreeHz ((float) v)) + " Hz";
    };

    const String shapeId = ParamIDs::lfo (i, "Shape");
    auto stepShape = [this, shapeId] (int delta)
    {
        const int n = lfoShapeNames().size();
        const int current = roundToInt (value (shapeId));
        processor.setParameterValue (shapeId, (float) (((current + delta) % n + n) % n));
    };
    c.prevShape.onClick = [stepShape] { stepShape (-1); };
    c.nextShape.onClick = [stepShape] { stepShape (1); };
    setHelp (c.prevShape, "SHAPE  -  previous LFO waveform");
    setHelp (c.nextShape, "SHAPE  -  next LFO waveform");
    addAndMakeVisible (c.prevShape);
    addAndMakeVisible (c.nextShape);

    const String destId = ParamIDs::lfo (i, "Dest");
    const auto names = lfoDestNames();
    for (int d = 0; d < NumLfoDests; ++d)
    {
        auto& b = c.dest[(size_t) d];
        b.setButtonText (names[d].toUpperCase());
        b.onClick = [this, destId, d] { processor.setParameterValue (destId, (float) d); };
        setHelp (b, "DESTINATION  -  LFO " + String (i + 1) + " modulates " + names[d]);
        addAndMakeVisible (b);
    }
    addAndMakeVisible (c.readout);
    setHelp (c.readout, "LFO " + String (i + 1) + "  -  routing and depth");
}

void LayersPage::paint (Graphics& g)
{
    AcidLookAndFeel::drawPanel (g, { 8.0f, 0.0f, 250.0f, 150.0f }, "Osc B");
    AcidLookAndFeel::drawPanel (g, { 264.0f, 0.0f, 290.0f, 150.0f }, "Unison");
    AcidLookAndFeel::drawPanel (g, { 560.0f, 0.0f, 432.0f, 150.0f }, "Amp Envelope");
    AcidLookAndFeel::drawPanel (g, { 8.0f, 156.0f, 984.0f, 150.0f }, "Echo");
    AcidLookAndFeel::drawPanel (g, { 8.0f, 312.0f, 489.0f, (float) getHeight() - 312.0f }, "LFO 1");
    AcidLookAndFeel::drawPanel (g, { 503.0f, 312.0f, 489.0f, (float) getHeight() - 312.0f }, "LFO 2");

    AcidLookAndFeel::drawSmallLabel (g, "Phase Mode", { 280.0f, 42.0f, 86.0f, 12.0f }, Justification::centredLeft);
    for (int i = 0; i < 2; ++i)
    {
        const float x0 = i == 0 ? 8.0f : 503.0f;
        AcidLookAndFeel::drawSmallLabel (g, "Retrig", { x0 + 16.0f, 440.0f, 120.0f, 12.0f }, Justification::centredLeft);
        AcidLookAndFeel::drawSmallLabel (g, "Destination", { x0, 492.0f, 489.0f, 12.0f });
    }
}

void LayersPage::layoutLfo (int i, int x0)
{
    auto& c = lfo[i];
    c.rate.setBounds (x0 + 16, 336, 76, 86);
    c.depth.setBounds (x0 + 96, 336, 76, 86);
    c.phase.setBounds (x0 + 176, 336, 76, 86);
    c.preview.setBounds (x0 + 262, 336, 211, 96);
    c.retrig.setBounds (x0 + 16, 454, 110, 22);
    c.prevShape.setBounds (x0 + 262, 446, 28, 24);
    c.sync.setBounds (x0 + 296, 446, 72, 24);
    c.mirror.setBounds (x0 + 372, 446, 66, 24);
    c.nextShape.setBounds (x0 + 445, 446, 28, 24);
    for (int d = 0; d < NumLfoDests; ++d)
        c.dest[(size_t) d].setBounds (x0 + 14 + d * 93, 508, 88, 24);
    c.readout.setBounds (x0 + 14, 544, 460, 26);
}

void LayersPage::resized()
{
    oscBLevel.setBounds (22, 24, 72, 84);
    oscBTune.setBounds (96, 24, 72, 84);
    oscBDetune.setBounds (170, 24, 72, 84);
    oscBLockButton.setBounds (22, 116, 220, 24);

    unisonFree.setBounds (280, 58, 86, 24);
    voices.setBounds (382, 32, 72, 84);
    spread.setBounds (466, 32, 72, 84);

    ampAttack.setBounds (576, 32, 72, 84);
    ampDecay.setBounds (656, 32, 72, 84);
    ampRelease.setBounds (736, 32, 72, 84);
    ampLcd.setBounds (824, 58, 154, 26);

    echoSync.setBounds (24, 212, 72, 24);
    const int ex = 120, step = 104;
    AcidKnob* echoKnobs[] = { &echoTime, &echoMix, &echoFeedback, &echoLowCut, &echoHighCut, &echoWidth };
    for (int i = 0; i < 6; ++i)
        echoKnobs[i]->setBounds (ex + i * step, 184, 84, 88);
    echoLcd.setBounds (764, 212, 212, 26);

    layoutLfo (0, 8);
    layoutLfo (1, 503);
}

void LayersPage::update()
{
    // unison phase mode text
    unisonFree.setButtonText (value (ParamIDs::unisonFree) > 0.5f ? "FREE" : "RESET");

    ampLcd.setContent ("A/D/R", formatMs (ampAttackToMs (value (ParamIDs::ampAttack))) + " / "
                                + (ampDecayHolds (value (ParamIDs::ampDecay)) ? String ("Hold") : formatMs (ampDecayToMs (value (ParamIDs::ampDecay))))
                                + " / " + formatMs (ampReleaseToMs (value (ParamIDs::ampRelease))));

    const float t = value (ParamIDs::echoTime);
    if (value (ParamIDs::echoSync) > 0.5f)
    {
        const auto& d = echoDivisions()[(size_t) divisionIndex (t, (int) echoDivisions().size())];
        echoLcd.setContent ("TIME " + String (d.name), formatMs ((float) (d.beats * 60000.0 / jmax (20.0f, processor.gui.bpm.load()))));
    }
    else
    {
        echoLcd.setContent ("TIME", formatMs (echoFreeMs (t)));
    }
    if (value (ParamIDs::echoOn) < 0.5f)
        echoLcd.setContent ("ECHO OFF", "tail rings out");

    for (int i = 0; i < 2; ++i)
    {
        auto& c = lfo[i];
        const int shape = roundToInt (value (ParamIDs::lfo (i, "Shape")));
        const float depth = value (ParamIDs::lfo (i, "Depth"));
        c.preview.setState (shape, processor.gui.lfoPhase[i].load(), depth / 100.0f, value (ParamIDs::lfo (i, "Mirror")) > 0.5f);

        const int dest = roundToInt (value (ParamIDs::lfo (i, "Dest")));
        for (int d = 0; d < NumLfoDests; ++d)
            c.dest[(size_t) d].setToggleState (d == dest, dontSendNotification);
        c.readout.setContent ("LFO " + String (i + 1) + "  >  " + lfoDestNames()[dest],
                              depth == 0.0f ? String ("off (depth 0)") : (depth > 0 ? "+" : "") + formatNumber (depth) + " %");
        c.rate.repaint();
    }
    echoTime.repaint();
}
