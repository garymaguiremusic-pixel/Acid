#include "Page303.h"

using namespace juce;
using namespace acid;

namespace
{
    constexpr int kRowH = 117, kRow2Y = 123, kOptX = 314, kOptW = 678;
    const std::pair<const char*, const char*> kOptions[] = {
        { ParamIDs::b3AccurateGate, "Accurate Gate" }, { ParamIDs::b3VcaNoise, "VCA Noise" },
        { ParamIDs::b3Vibrato, "Vibrato" },             { ParamIDs::b3PhaseReset, "Phase Reset" },
        { ParamIDs::b3SlowSlide, "Slow Slide" },        { ParamIDs::b3WeakAccentCap, "Weak Accent" },
        { ParamIDs::b3InfSustain, "Inf Sustain" },      { ParamIDs::b3SoftAccent, "Soft Accent" } };
}

Page303::Page303 (AcidAudioProcessor& p)
    : PageBase (p),
      waveButtons (*p.apvts.getParameter (ParamIDs::b3Wave), b3WaveNames(), {}),
      modelButtons (*p.apvts.getParameter (ParamIDs::b3Model), b3ModelNames(), {}),
      octave (*p.apvts.getParameter (ParamIDs::b3Octave), "303 OCTAVE")
{
    using namespace ParamIDs;
    addAndMakeVisible (waveButtons);
    setHelp (waveButtons, "WAVEFORM  -  " + helpForParameter (b3Wave));
    addAndMakeVisible (octave);
    attach (velocityButton, b3VelLink);
    attach (tuning, b3Tuning);
    attach (cutoff, b3Cutoff);
    attach (resonance, b3Resonance);
    attach (envMod, b3EnvMod);
    attach (decay, b3Decay);
    attach (accent, b3Accent);
    attach (volume, b3Volume);

    addAndMakeVisible (modelButtons);
    setHelp (modelButtons, "VCO MODEL  -  " + helpForParameter (b3Model));
    for (auto [id, name] : kOptions)
        attach (*options.add (new TextButton (name)), id);
}

void Page303::paint (Graphics& g)
{
    AcidLookAndFeel::drawPanel (g, { 8.0f, 0.0f, 984.0f, (float) kRowH }, "303");
    AcidLookAndFeel::drawPanel (g, { (float) kOptX, (float) kRow2Y, (float) kOptW, (float) kRowH }, "303 Options");
    AcidLookAndFeel::drawSmallLabel (g, "VCO Model", { (float) kOptX + 12.0f, (float) kRow2Y + 30.0f, 180.0f, 12.0f }, Justification::centredLeft);
}

void Page303::resized()
{
    // row 1: Waveform / Octave / Velocity, then the seven knobs
    waveButtons.setBounds (16, 28, 90, 22);
    octave.setBounds (16, 56, 90, 22);
    velocityButton.setBounds (16, 84, 90, 22);
    AcidKnob* knobs[] = { &tuning, &cutoff, &resonance, &envMod, &decay, &accent, &volume };
    const int step = (984 - 120) / 7;
    for (int i = 0; i < 7; ++i)
        knobs[i]->setBounds (124 + i * step + (step - 70) / 2, 24, 70, 88);

    // row 2 (right of the 303's Amp Envelope): VCO model, then the options in two rows of four
    modelButtons.setBounds (kOptX + 12, kRow2Y + 46, 180, 24);
    const int ox = kOptX + 206, bw = (kOptX + kOptW - 12 - ox - 3 * 6) / 4;
    for (int i = 0; i < options.size(); ++i)
        options[i]->setBounds (ox + (i % 4) * (bw + 6), kRow2Y + 32 + (i / 4) * 34, bw, 26);
}
