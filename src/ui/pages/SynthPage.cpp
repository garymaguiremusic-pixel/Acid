#include "SynthPage.h"

using namespace juce;
using namespace acid;

SynthPage::SynthPage (AcidAudioProcessor& p)
    : PageBase (p),
      octaveA (*p.apvts.getParameter (ParamIDs::oscAOctave), "OSC A OCTAVE"),
      filterModeButtons (*p.apvts.getParameter (ParamIDs::filterMode), filterModeNames(), {}),
      page303 (p),
      sequencer (p)
{
    using namespace ParamIDs;

    // OSC
    waveButton.onClick = [this] { processor.setParameterValue (ParamIDs::wave, value (ParamIDs::wave) > 0.5f ? 0.0f : 1.0f); };
    setHelp (waveButton, "WAVE  -  " + helpForParameter (wave) + " Click to switch.");
    addAndMakeVisible (waveButton);
    addAndMakeVisible (waveDisplay);
    setHelp (waveDisplay, "WAVE  -  the oscillator waveform, drawn from the DSP");
    attach (retroButton, retro);
    attach (lockButton, oscALock);

    // FILTER
    attach (cutoffKnob, cutoff);
    attach (resoKnob, resonance);
    attach (cutoffLowKnob, cutoffLow);
    attach (driveKnob, drive);
    attach (bassKnob, bass);
    attach (trebleKnob, treble);
    attach (transistorButton, transistor);
    addAndMakeVisible (filterModeButtons);
    setHelp (filterModeButtons, "FILTER TYPE  -  " + helpForParameter (filterMode));
    addAndMakeVisible (cutoffLcd);
    addAndMakeVisible (resoLcd);
    attach (velLinkButton, filterVelLink);
    attach (velAmountKnob, filterVelAmount);
    addAndMakeVisible (velocityLcd);
    setHelp (velocityLcd, "VELOCITY  -  the velocity Vel Link is following: the step's Velocity lane, or the key you played");

    // ENVELOPE
    attach (envModKnob, envMod);
    attach (decayKnob, decay);
    attach (accentKnob, accent);
    attach (envAttackKnob, envAttack);
    attach (accDecayKnob, accDecay);
    attach (accSweepKnob, accSweep);
    addAndMakeVisible (envMeter);
    setHelp (envMeter, "ENV / ACC  -  the filter envelope and the accent-sweep charge, live");

    addAndMakeVisible (octaveA);
    addChildComponent (page303);
    addAndMakeVisible (sequencer);

    // everything of the Acid sections (hidden while the 303 is shown)
    for (auto* c : getChildren())
        if (c != &page303 && c != &sequencer)
            acidControls.push_back (c);
}

void SynthPage::paint (Graphics& g)
{
    if (shownEngine == 1)
        return;     // the 303 section draws itself
    AcidLookAndFeel::drawPanel (g, { 8.0f, 0.0f, 150.0f, 206.0f }, "Osc");
    AcidLookAndFeel::drawPanel (g, { 164.0f, 0.0f, 500.0f, 206.0f }, "Filter");
    AcidLookAndFeel::drawPanel (g, { 670.0f, 0.0f, 322.0f, 206.0f }, "Envelope");

    // the Velocity column of the Filter
    g.setColour (Theme::get().bevelDark);
    g.drawVerticalLine (582, 24.0f, 198.0f);
    g.setColour (Colours::white.withAlpha (0.07f));
    g.drawVerticalLine (583, 24.0f, 198.0f);
    AcidLookAndFeel::drawSmallLabel (g, "Velocity", { 588.0f, 24.0f, 70.0f, 12.0f });
    AcidLookAndFeel::drawSmallLabel (g, "Wave", { 20.0f, 22.0f, 126.0f, 12.0f }, Justification::centredLeft);
}

void SynthPage::resized()
{
    // OSC
    waveButton.setBounds (20, 36, 126, 22);
    waveDisplay.setBounds (20, 62, 126, 58);
    octaveA.setBounds (20, 124, 126, 22);
    retroButton.setBounds (20, 150, 126, 22);
    lockButton.setBounds (20, 176, 126, 22);
    page303.setBounds (0, 0, getWidth(), 206);

    // FILTER
    cutoffKnob.setBounds (178, 24, 116, 116);
    resoKnob.setBounds (304, 24, 116, 116);
    cutoffLcd.setBounds (178, 148, 116, 22);
    resoLcd.setBounds (304, 148, 116, 22);
    cutoffLowKnob.setBounds (430, 22, 72, 76);
    driveKnob.setBounds (504, 22, 72, 76);
    bassKnob.setBounds (430, 100, 72, 76);
    trebleKnob.setBounds (504, 100, 72, 76);
    filterModeButtons.setBounds (178, 176, 116, 22);      // beside the cutoff
    transistorButton.setBounds (304, 176, 116, 22);
    velLinkButton.setBounds (590, 40, 66, 22);
    velAmountKnob.setBounds (586, 68, 74, 80);
    velocityLcd.setBounds (588, 152, 70, 22);

    // ENVELOPE: Env Mod, Decay, Accent / Env Attack, Acc Decay, Acc Sweep, with the live meter
    const int ex = 676, step = 80;
    envModKnob.setBounds (ex, 22, 78, 84);
    decayKnob.setBounds (ex + step, 22, 78, 84);
    accentKnob.setBounds (ex + step * 2, 22, 78, 84);
    envAttackKnob.setBounds (ex, 112, 78, 84);
    accDecayKnob.setBounds (ex + step, 112, 78, 84);
    accSweepKnob.setBounds (ex + step * 2, 112, 78, 84);
    envMeter.setBounds (ex + step * 3 + 2, 70, 70, 70);

    sequencer.setBounds (0, 212, getWidth(), getHeight() - 212);
}

void SynthPage::update()
{
    const int engine = roundToInt (value (ParamIDs::engine));
    if (engine != shownEngine)
    {
        shownEngine = engine;
        page303.setVisible (engine == 1);
        for (auto* c : acidControls)
            c->setVisible (engine == 0);
        repaint();
    }

    auto& gui = processor.gui;

    const bool square = value (ParamIDs::wave) > 0.5f;
    waveButton.setButtonText (square ? "SQUARE" : "SAW");
    waveDisplay.setMorph (square ? 1.0f : 0.0f);
    cutoffLcd.setContent ("CUTOFF", formatHz (cutoffToHz (value (ParamIDs::cutoff))));
    resoLcd.setContent ("RESONANCE", formatNumber (value (ParamIDs::resonance)));
    envMeter.setLevels (gui.filterEnv.load(), jlimit (0.0f, 1.0f, gui.accentCharge.load()));

    const bool velLink = value (ParamIDs::filterVelLink) > 0.5f;
    velAmountKnob.setEnabled (velLink);
    velAmountKnob.setAlpha (velLink ? 1.0f : 0.4f);
    velocityLcd.setContent ("VEL", velLink ? String (gui.lastVelocity.load()) : String ("off"));

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
