#include "AmpEnvPanel.h"

using namespace juce;
using namespace acid;

AmpEnvPanel::AmpEnvPanel (AcidAudioProcessor& p, const char* a, const char* d, const char* r, const char* s)
    : PageBase (p), attackId (a), decayId (d), releaseId (r), syncId (s)
{
    attach (attack, attackId);
    attach (decay, decayId);
    attach (release, releaseId);
    attach (syncButton, syncId);
    addAndMakeVisible (lcd);
    setHelp (lcd, "AMP ENVELOPE  -  attack / decay / release");
    for (auto [knob, id] : { std::pair<AcidKnob*, const char*> { &attack, attackId }, { &decay, decayId }, { &release, releaseId } })
        knob->displayValue = [this, id = id] (double v) { return text (id, (float) v); };
}

String AmpEnvPanel::text (const char* id, float v) const
{
    if (id == decayId && ampDecayHolds (v))
        return "Hold";
    if (value (syncId) > 0.5f)
        return ampDivisions()[(size_t) divisionIndex (v, (int) ampDivisions().size())].name;
    return describeParameter (id, v);
}

void AmpEnvPanel::paint (Graphics& g)
{
    AcidLookAndFeel::drawPanel (g, getLocalBounds().toFloat(), "Amp Envelope");
}

void AmpEnvPanel::resized()
{
    // three knobs, then Sync over the readout; a narrow panel puts Sync in its title bar
    const int h = getHeight();
    const bool narrow = getWidth() < 340;
    const int knobW = narrow ? jmin (60, (getWidth() - 20) / 3) : 60, knobH = jmin (80, h - 30);
    int x = narrow ? (getWidth() - 3 * knobW - 4) / 2 : 8;
    for (auto* k : { &attack, &decay, &release })
    {
        k->setBounds (x, 26, knobW, knobH);
        x += knobW + 2;
    }
    lcd.setVisible (! narrow);
    if (narrow)
        syncButton.setBounds (getWidth() - 50, 2, 44, 16);
    else
    {
        syncButton.setBounds (x + 14, 34, 72, 22);
        lcd.setBounds (x + 14, 66, getWidth() - x - 26, 22);
    }
}

void AmpEnvPanel::update()
{
    lcd.setContent ("A/D/R", text (attackId, value (attackId)) + " / " + text (decayId, value (decayId))
                              + " / " + text (releaseId, value (releaseId)));
}
