#include "Page303.h"

using namespace juce;
using namespace acid;

Page303::Page303 (AcidAudioProcessor& p)
    : PageBase (p),
      waveButtons (*p.apvts.getParameter (ParamIDs::b3Wave), b3WaveNames(), {}),
      octave (*p.apvts.getParameter (ParamIDs::b3Octave), "303 OCTAVE")
{
    using namespace ParamIDs;
    addAndMakeVisible (waveButtons);
    setHelp (waveButtons, "WAVEFORM  -  " + helpForParameter (b3Wave));
    addAndMakeVisible (octave);
    optionsButton.onClick = [this] { showOptions(); };
    setHelp (optionsButton, "OPTIONS  -  VCO model, gate, VCA noise, vibrato, phase reset and the aged-part settings");
    addAndMakeVisible (optionsButton);

    attach (velocityButton, b3VelLink);
    attach (tuning, b3Tuning);
    attach (cutoff, b3Cutoff);
    attach (resonance, b3Resonance);
    attach (envMod, b3EnvMod);
    attach (decay, b3Decay);
    attach (accent, b3Accent);
    attach (volume, b3Volume);
}

void Page303::showOptions()
{
    using namespace ParamIDs;
    auto isOn = [this] (const char* id) { return value (id) > 0.5f; };
    auto toggle = [this] (const char* id) { processor.setParameterValue (id, value (id) > 0.5f ? 0.0f : 1.0f); };

    PopupMenu models;
    const auto names = b3ModelNames();
    for (int i = 0; i < names.size(); ++i)
        models.addItem (names[i], true, roundToInt (value (b3Model)) == i, [this, i] { processor.setParameterValue (ParamIDs::b3Model, (float) i); });

    PopupMenu m;
    m.addSectionHeader ("303 Options");
    m.addSubMenu ("VCO Model: " + names[roundToInt (value (b3Model))], models);
    m.addSeparator();
    const std::pair<const char*, const char*> switches[] = {
        { b3AccurateGate, "Accurate Gate" }, { b3VcaNoise, "VCA Noise" }, { b3Vibrato, "Vibrato" },
        { b3PhaseReset, "Phase Reset" } };
    for (auto [id, name] : switches)
        m.addItem (name, true, isOn (id), [toggle, id] { toggle (id); });
    m.addSeparator();
    m.addSectionHeader ("Aged / modified parts");
    const std::pair<const char*, const char*> parts[] = {
        { b3SlowSlide, "Slow Slide" }, { b3WeakAccentCap, "Weak Accent Cap" },
        { b3InfSustain, "Infinite Sustain" }, { b3SoftAccent, "Soft Accent" } };
    for (auto [id, name] : parts)
        m.addItem (name, true, isOn (id), [toggle, id] { toggle (id); });

    m.showMenuAsync (PopupMenu::Options().withTargetComponent (&optionsButton));
}

void Page303::paint (Graphics& g)
{
    AcidLookAndFeel::drawPanel (g, { 8.0f, 0.0f, 984.0f, 206.0f }, "303");
    AcidLookAndFeel::drawSmallLabel (g, "Waveform", { 26.0f, 40.0f, 96.0f, 12.0f }, Justification::centredLeft);
}

void Page303::resized()
{
    waveButtons.setBounds (24, 56, 96, 26);
    octave.setBounds (24, 94, 96, 24);
    optionsButton.setBounds (24, 132, 96, 26);
    velocityButton.setBounds (24, 166, 96, 26);

    AcidKnob* knobs[] = { &tuning, &cutoff, &resonance, &envMod, &decay, &accent, &volume };
    for (int i = 0; i < 7; ++i)
        knobs[i]->setBounds (138 + i * 122, 34, 112, 140);
}
