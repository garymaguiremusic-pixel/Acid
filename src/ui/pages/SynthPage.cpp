#include "SynthPage.h"

using namespace juce;
using namespace acid;

namespace
{
    constexpr int kRowH = 117, kRow2Y = 123;
    constexpr int kOscX = 8, kOscW = 196, kFiltX = 210, kFiltW = 430, kEnvX = 646, kEnvW = 346;
    constexpr int kKnobY = 26, kKnobW = 54, kKnobH = 78;
}

SynthPage::SynthPage (AcidAudioProcessor& p)
    : PageBase (p),
      octaveA (*p.apvts.getParameter (ParamIDs::oscAOctave), "OSC 1 OCTAVE"),
      filterModeButtons (*p.apvts.getParameter (ParamIDs::filterMode), filterModeNames(), {}),
      layerRow (p),
      page303 (p),
      amp303 (p, ParamIDs::b3AmpAttack, ParamIDs::b3AmpDecay, ParamIDs::b3AmpRelease, ParamIDs::b3AmpSync),
      fx (p),
      sequencer (p)
{
    using namespace ParamIDs;

    // Osc 1
    waveButton.onClick = [this] { processor.setParameterValue (ParamIDs::wave, value (ParamIDs::wave) > 0.5f ? 0.0f : 1.0f); };
    setHelp (waveButton, "WAVE  -  " + helpForParameter (wave) + " Click to switch.");
    addAndMakeVisible (waveButton);
    addAndMakeVisible (waveDisplay);
    setHelp (waveDisplay, "WAVE  -  the oscillator waveform, drawn from the DSP");
    addAndMakeVisible (octaveA);
    attach (retroButton, retro);
    attach (lockButton, oscALock);

    // Filter
    attach (cutoffKnob, cutoff);
    attach (resoKnob, resonance);
    attach (cutoffLowKnob, cutoffLow);
    attach (driveKnob, drive);
    attach (bassKnob, bass);
    attach (trebleKnob, treble);
    attach (transistorButton, transistor);
    addAndMakeVisible (filterModeButtons);
    setHelp (filterModeButtons, "FILTER TYPE  -  " + helpForParameter (filterMode));

    // Envelope
    attach (envModKnob, envMod);
    attach (decayKnob, decay);
    attach (accentKnob, accent);
    attach (envAttackKnob, envAttack);
    attach (accDecayKnob, accDecay);
    attach (accSweepKnob, accSweep);

    for (auto* c : getChildren())
        acidControls.push_back (c);

    addChildComponent (layerRow);
    addChildComponent (page303);
    addChildComponent (amp303);
    addChildComponent (fx);
    addAndMakeVisible (sequencer);
}

void SynthPage::paint (Graphics& g)
{
    if (view != 0)
        return;     // B and FX draw themselves
    AcidLookAndFeel::drawPanel (g, { (float) kOscX, 0.0f, (float) kOscW, (float) kRowH }, "Osc 1");
    AcidLookAndFeel::drawPanel (g, { (float) kFiltX, 0.0f, (float) kFiltW, (float) kRowH }, "Filter");
    AcidLookAndFeel::drawPanel (g, { (float) kEnvX, 0.0f, (float) kEnvW, (float) kRowH }, "Envelope");
}

void SynthPage::resized()
{
    auto knobs = [] (int x, std::initializer_list<AcidKnob*> list, int w = kKnobW)
    {
        for (auto* k : list)
        {
            k->setBounds (x, kKnobY, w, kKnobH);
            x += w + 2;
        }
        return x;
    };

    // Osc 1: Wave / Octave / Retro on the left, the wave display over Phase Lock on the right
    waveButton.setBounds (kOscX + 8, 28, 84, 22);
    octaveA.setBounds (kOscX + 8, 56, 84, 22);
    retroButton.setBounds (kOscX + 8, 84, 84, 22);   // (the stepper shows just the number at this width)
    waveDisplay.setBounds (kOscX + 98, 28, 90, 50);
    lockButton.setBounds (kOscX + 98, 84, 90, 22);

    // Filter: Cutoff and Resonance (a little bigger), four small knobs, then the type and Transistor
    int x = knobs (kFiltX + 6, { &cutoffKnob, &resoKnob }, 64);
    x = knobs (x + 2, { &cutoffLowKnob, &driveKnob, &bassKnob, &trebleKnob }, 50);
    filterModeButtons.setBounds (x + 4, 34, kFiltX + kFiltW - x - 12, 22);
    transistorButton.setBounds (x + 4, 66, kFiltX + kFiltW - x - 12, 22);

    // Envelope: six knobs in a row
    knobs (kEnvX + 6, { &envModKnob, &decayKnob, &accentKnob, &envAttackKnob, &accDecayKnob, &accSweepKnob }, 54);

    // row 2 and the other sections
    layerRow.setBounds (0, kRow2Y, getWidth(), LayerRow::kHeight);
    page303.setBounds (0, 0, getWidth(), kSectionH);
    amp303.setBounds (8, kRow2Y, 300, LayerRow::kHeight);
    fx.setBounds (0, 0, getWidth(), kSectionH);

    sequencer.setBounds (0, kSectionH + 6, getWidth(), getHeight() - kSectionH - 6);
}

void SynthPage::setView (int v)
{
    view = jlimit (0, 2, v);
    update();
}

void SynthPage::update()
{
    if (view != shownView)
    {
        shownView = view;
        for (auto* c : acidControls)
            c->setVisible (view == 0);
        layerRow.setVisible (view == 0);
        page303.setVisible (view == 1);
        amp303.setVisible (view == 1);
        fx.setVisible (view == 2);
        repaint();
    }
    if (view == 0)      layerRow.update();
    else if (view == 1) amp303.update();
    else                fx.update();

    const bool square = value (ParamIDs::wave) > 0.5f;
    waveButton.setButtonText (square ? "SQUARE" : "SAW");
    waveDisplay.setMorph (square ? 1.0f : 0.0f);

    sequencer.update();
}
