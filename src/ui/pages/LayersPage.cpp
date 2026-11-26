#include "LayersPage.h"

using namespace juce;
using namespace acid;

namespace
{
    // two columns, lined up with the LFO panels below
    constexpr float kLeftX = 8.0f, kRightX = 503.0f, kColW = 489.0f;
    constexpr int kSmallH = 118;                    // Unison / Amp Envelope (stacked on the right)
    constexpr int kTopH = kSmallH * 2 + 6;          // Osc B (left) is as tall as both
    constexpr int kEchoY = kTopH + 6, kEchoH = 150;
    constexpr int kLfoY = kEchoY + kEchoH + 6;
    constexpr int kLfoPad = 52;                     // extra space above the LFO contents (centres them)
}

LayersPage::LayersPage (AcidAudioProcessor& p)
    : PageBase (p), octaveB (*p.apvts.getParameter (ParamIDs::oscBOctave), "OSC B OCTAVE"),
      fmDirection (*p.apvts.getParameter (ParamIDs::fmDirection), fmDirectionNames(), {})
{
    using namespace ParamIDs;
    attach (oscBLevel, ParamIDs::oscBLevel);
    attach (oscBTune, ParamIDs::oscBTune);
    attach (oscBDetune, ParamIDs::oscBDetune);
    attach (oscBLockButton, ParamIDs::oscBLock);
    addAndMakeVisible (octaveB);
    attach (syncButton, ParamIDs::oscSync);
    attach (fmAmount, ParamIDs::fmAmount);
    addAndMakeVisible (fmDirection);
    setHelp (fmDirection, "FM DIRECTION  -  " + helpForParameter (ParamIDs::fmDirection));
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
    AcidLookAndFeel::drawPanel (g, { kLeftX, 0.0f, kColW, (float) kTopH }, "Osc B");
    AcidLookAndFeel::drawPanel (g, { kRightX, 0.0f, kColW, (float) kSmallH }, "Unison");
    AcidLookAndFeel::drawPanel (g, { kRightX, (float) (kSmallH + 6), kColW, (float) kSmallH }, "Amp Envelope");
    AcidLookAndFeel::drawPanel (g, { 8.0f, (float) kEchoY, 984.0f, (float) kEchoH }, "Echo");
    AcidLookAndFeel::drawPanel (g, { kLeftX, (float) kLfoY, kColW, (float) (getHeight() - kLfoY) }, "LFO 1");
    AcidLookAndFeel::drawPanel (g, { kRightX, (float) kLfoY, kColW, (float) (getHeight() - kLfoY) }, "LFO 2");

    // Osc B: a label over each control of the lower row, under its knob
    const char* labels[] = { "Octave", "Phase", "Sync", "FM Direction" };
    for (int i = 0; i < 4; ++i)
        AcidLookAndFeel::drawSmallLabel (g, labels[i], { kLeftX + 10.0f + (float) i * 117.0f, 150.0f, 117.0f, 12.0f });
    g.setColour (Theme::get().bevelDark);
    g.drawHorizontalLine (140, kLeftX + 14.0f, kLeftX + kColW - 14.0f);
    g.setColour (Colours::white.withAlpha (0.07f));
    g.drawHorizontalLine (141, kLeftX + 14.0f, kLeftX + kColW - 14.0f);

    AcidLookAndFeel::drawSmallLabel (g, "Phase Mode", { kRightX + 24.0f, 42.0f, 110.0f, 12.0f });
    for (int i = 0; i < 2; ++i)
    {
        const float x0 = i == 0 ? kLeftX : kRightX;
        const float y = (float) (kLfoY - 312 + kLfoPad);
        AcidLookAndFeel::drawSmallLabel (g, "Retrig", { x0 + 16.0f, 440.0f + y, 120.0f, 12.0f }, Justification::centredLeft);
        AcidLookAndFeel::drawSmallLabel (g, "Destination", { x0, 492.0f + y, kColW, 12.0f });
    }
}

void LayersPage::layoutLfo (int i, int x0)
{
    auto& c = lfo[i];
    const int y = kLfoY - 312 + kLfoPad;
    c.rate.setBounds (x0 + 16, 336 + y, 76, 86);
    c.depth.setBounds (x0 + 96, 336 + y, 76, 86);
    c.phase.setBounds (x0 + 176, 336 + y, 76, 86);
    c.preview.setBounds (x0 + 262, 336 + y, 211, 96);
    c.retrig.setBounds (x0 + 16, 454 + y, 110, 22);
    c.prevShape.setBounds (x0 + 262, 446 + y, 28, 24);
    c.sync.setBounds (x0 + 296, 446 + y, 72, 24);
    c.mirror.setBounds (x0 + 372, 446 + y, 66, 24);
    c.nextShape.setBounds (x0 + 445, 446 + y, 28, 24);
    for (int d = 0; d < NumLfoDests; ++d)
        c.dest[(size_t) d].setBounds (x0 + 14 + d * 93, 508 + y, 88, 24);
    c.readout.setBounds (x0 + 14, 544 + y, 460, 26);
}

void LayersPage::resized()
{
    // Osc B: four knobs across, the octave / phase / sync / FM direction row under them
    const int ox = (int) kLeftX + 10, col = 117;
    AcidKnob* oscKnobs[] = { &oscBLevel, &oscBTune, &oscBDetune, &fmAmount };
    for (int i = 0; i < 4; ++i)
        oscKnobs[i]->setBounds (ox + i * col + (col - 88) / 2, 32, 88, 96);
    octaveB.setBounds (ox + 4, 168, col - 8, 26);
    oscBLockButton.setBounds (ox + col + 4, 168, col - 8, 26);
    syncButton.setBounds (ox + col * 2 + 4, 168, col - 8, 26);
    fmDirection.setBounds (ox + col * 3 + 4, 168, col - 8, 26);

    // Unison: phase mode, voices, spread
    const int rx = (int) kRightX;
    unisonFree.setBounds (rx + 24, 58, 110, 26);
    voices.setBounds (rx + 190, 26, 80, 86);
    spread.setBounds (rx + 300, 26, 80, 86);

    // Amp Envelope: attack, decay, release and the readout
    const int ay = kSmallH + 6;
    ampAttack.setBounds (rx + 14, ay + 26, 80, 86);
    ampDecay.setBounds (rx + 104, ay + 26, 80, 86);
    ampRelease.setBounds (rx + 194, ay + 26, 80, 86);
    ampLcd.setBounds (rx + 290, ay + 56, 184, 26);

    // Echo
    const int ey = kEchoY - 156;
    echoSync.setBounds (24, 212 + ey, 72, 24);
    const int ex = 120, step = 104;
    AcidKnob* echoKnobs[] = { &echoTime, &echoMix, &echoFeedback, &echoLowCut, &echoHighCut, &echoWidth };
    for (int i = 0; i < 6; ++i)
        echoKnobs[i]->setBounds (ex + i * step, 184 + ey, 84, 88);
    echoLcd.setBounds (764, 212 + ey, 212, 26);

    layoutLfo (0, (int) kLeftX);
    layoutLfo (1, (int) kRightX);
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
