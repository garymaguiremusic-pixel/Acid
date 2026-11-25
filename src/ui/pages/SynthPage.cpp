#include "SynthPage.h"

using namespace juce;
using namespace acid;

SynthPage::SynthPage (AcidAudioProcessor& p) : PageBase (p), sequencer (p)
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
    attach (envAttackKnob, envAttack);
    attach (accDecayKnob, accDecay);
    attach (accSweepKnob, accSweep);
    addAndMakeVisible (envMeter);
    setHelp (envMeter, "ENV / ACC  -  the filter envelope and the accent-sweep charge, live");

    addAndMakeVisible (sequencer);
}

void SynthPage::paint (Graphics& g)
{
    AcidLookAndFeel::drawPanel (g, { 8.0f, 0.0f, 150.0f, 206.0f }, "Osc");
    AcidLookAndFeel::drawPanel (g, { 164.0f, 0.0f, 420.0f, 206.0f }, "Filter");
    AcidLookAndFeel::drawPanel (g, { 590.0f, 0.0f, 402.0f, 206.0f }, "Envelope");
    AcidLookAndFeel::drawSmallLabel (g, "Wave", { 20.0f, 22.0f, 126.0f, 12.0f }, Justification::centredLeft);
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

    // ENVELOPE: Env Mod, Decay, Accent / Env Attack, Acc Decay, Acc Sweep, with the live meter
    const int ex = 604, step = 96;
    envModKnob.setBounds (ex, 22, 84, 84);
    decayKnob.setBounds (ex + step, 22, 84, 84);
    accentKnob.setBounds (ex + step * 2, 22, 84, 84);
    envAttackKnob.setBounds (ex, 112, 84, 84);
    accDecayKnob.setBounds (ex + step, 112, 84, 84);
    accSweepKnob.setBounds (ex + step * 2, 112, 84, 84);
    envMeter.setBounds (ex + step * 3, 70, 92, 70);

    sequencer.setBounds (0, 212, getWidth(), getHeight() - 212);
}

void SynthPage::update()
{
    auto& gui = processor.gui;

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

    sequencer.update();
}
