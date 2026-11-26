#include "LayerRow.h"

using namespace juce;
using namespace acid;

namespace
{
    // Osc 2 | Sync / FM | Unison | Velocity | Amp Envelope
    constexpr int kOscX = 8,    kOscW = 266;
    constexpr int kSyncX = 280, kSyncW = 214;
    constexpr int kUniX = 500,  kUniW = 170;
    constexpr int kVelX = 676,  kVelW = 120;
    constexpr int kAmpX = 802,  kAmpW = 190;
    constexpr int kKnobY = 26, kKnobW = 54, kKnobH = 78;
    constexpr int kBtnW = 82, kBtnH = 22, kBtn1Y = 34, kBtn2Y = 66;
}

LayerRow::LayerRow (AcidAudioProcessor& p)
    : PageBase (p),
      octaveB (*p.apvts.getParameter (ParamIDs::oscBOctave), "OSC 2 OCTAVE"),
      fmDirection (*p.apvts.getParameter (ParamIDs::fmDirection), fmDirectionNames(), {}),
      amp (p, ParamIDs::ampAttack, ParamIDs::ampDecay, ParamIDs::ampRelease, ParamIDs::ampSync)
{
    attach (oscBLevel, ParamIDs::oscBLevel);
    attach (oscBTune, ParamIDs::oscBTune);
    attach (oscBDetune, ParamIDs::oscBDetune);
    attach (oscBLockButton, ParamIDs::oscBLock);
    addAndMakeVisible (octaveB);

    attach (syncButton, ParamIDs::oscSync);
    attach (syncPitch, ParamIDs::oscSyncPitch);
    syncPitch.displayValue = [] (double v) { return "+" + String (roundToInt (v)) + " st"; };
    attach (fmAmount, ParamIDs::fmAmount);
    addAndMakeVisible (fmDirection);
    setHelp (fmDirection, "FM DIRECTION  -  " + helpForParameter (ParamIDs::fmDirection));

    attach (voices, ParamIDs::unisonVoices);
    attach (spread, ParamIDs::unisonSpread);
    attach (unisonFree, ParamIDs::unisonFree);

    attach (velLinkButton, ParamIDs::filterVelLink);
    attach (velAmount, ParamIDs::filterVelAmount);

    addAndMakeVisible (amp);
}

void LayerRow::paint (Graphics& g)
{
    const float h = (float) kHeight;
    AcidLookAndFeel::drawPanel (g, { (float) kOscX, 0.0f, (float) kOscW, h }, "Osc 2");
    AcidLookAndFeel::drawPanel (g, { (float) kSyncX, 0.0f, (float) kSyncW, h }, "Sync / FM");
    AcidLookAndFeel::drawPanel (g, { (float) kUniX, 0.0f, (float) kUniW, h }, "Unison");
    AcidLookAndFeel::drawPanel (g, { (float) kVelX, 0.0f, (float) kVelW, h }, "Velocity");
}

void LayerRow::resized()
{
    auto knobs = [] (int x, std::initializer_list<AcidKnob*> list)
    {
        for (auto* k : list)
        {
            k->setBounds (x, kKnobY, kKnobW, kKnobH);
            x += kKnobW + 2;
        }
        return x;
    };

    // Osc 2: three knobs, then Octave over Phase Lock
    int x = knobs (kOscX + 8, { &oscBLevel, &oscBTune, &oscBDetune });
    octaveB.setBounds (x + 6, kBtn1Y, kOscX + kOscW - x - 14, kBtnH);
    oscBLockButton.setBounds (x + 6, kBtn2Y, kOscX + kOscW - x - 14, kBtnH);

    // Sync / FM: Hard Sync over the FM direction, then the Sync and FM knobs
    syncButton.setBounds (kSyncX + 8, kBtn1Y, kBtnW, kBtnH);
    fmDirection.setBounds (kSyncX + 8, kBtn2Y, kBtnW, kBtnH);
    knobs (kSyncX + kBtnW + 16, { &syncPitch, &fmAmount });

    // Unison: Free / Reset, then Voices and Spread
    unisonFree.setBounds (kUniX + 8, 50, 50, kBtnH);
    knobs (kUniX + 62, { &voices, &spread });

    // Velocity: Vel Link over its amount
    velLinkButton.setBounds (kVelX + 8, kBtn1Y - 6, kVelW - 16, kBtnH);
    velAmount.setBounds (kVelX + (kVelW - kKnobW) / 2, 54, kKnobW, 58);

    amp.setBounds (kAmpX, 0, kAmpW, kHeight);
}

void LayerRow::update()
{
    unisonFree.setButtonText (value (ParamIDs::unisonFree) > 0.5f ? "FREE" : "RESET");
    const bool velLink = value (ParamIDs::filterVelLink) > 0.5f;
    velAmount.setEnabled (velLink);
    velAmount.setAlpha (velLink ? 1.0f : 0.4f);
    amp.update();
}
