#pragma once

#include "DspCommon.h"
#include "Oscillator.h"
#include "Ladder.h"
#include "Envelopes.h"

namespace acid
{
/** Block-rate settings for the voice, already in real units. */
struct VoiceSettings
{
    float tuneSemis       = 0.0f;
    float waveMorph       = 0.0f;     // 0 = saw, 1 = square
    bool  retro           = false;

    float cutoffHz        = 600.0f;
    float cutoffLowHz     = 200.0f;
    float resonance       = 0.5f;     // 0..1
    float envMod          = 0.5f;     // 0..1
    float decayMs         = 300.0f;
    float accDecayMs      = 200.0f;
    float envAttackMs     = 3.0f;
    float accent          = 0.5f;     // 0..1
    float accSweep        = 0.3f;     // 0..1
    float drive           = 0.0f;     // 0..1
    bool  transistor      = false;

    float oscBLevel       = 0.0f;     // 0..1
    float oscBTuneSemis   = 0.0f;
    float oscBDetuneCents = 8.0f;
    int   unisonVoices    = 1;
    float unisonSpread    = 0.25f;    // 0..1
    bool  unisonFree      = true;

    float ampAttackMs     = 0.25f;
    float ampDecayMs      = 1000.0f;
    bool  ampHold         = false;
    float ampReleaseMs    = 5.0f;
    float slideMs         = 60.0f;
};

/** Optional per-sample modulation (LFOs). Null pointers mean "no modulation". */
struct VoiceMod
{
    const float* cutoffOct  = nullptr;   // octaves
    const float* pitchSemis = nullptr;   // semitones
    const float* wave       = nullptr;   // added to the morph
    const float* resonance  = nullptr;   // added to resonance (0..1 scale)
    const float* volume     = nullptr;   // gain multiplier
};

/** Monophonic 303-style voice. Everything from the oscillators to the drive runs at
    twice the host sample rate and is decimated with a half-band filter. */
class Voice
{
public:
    static constexpr int kMaxUnison = 8;
    static constexpr float kEnvModOctaves = 5.0f;
    static constexpr float kAccentOctaves = 2.5f;
    static constexpr int kBaseNote = 36;          // C2: the lowest note of a pattern at octave 0
    static constexpr float kOutputTrim = 1.5f;    // output level (headroom for the tube and echo)

    void prepare (double sampleRate);
    void reset();
    void setSettings (const VoiceSettings& s);

    /** legato = slide: glide to the new pitch without retriggering the envelopes. */
    void noteOn (int midiNote, bool accent, bool legato);
    void noteOff();
    bool isGateOn() const noexcept { return gate; }

    /** Renders (adds nothing, overwrites) numSamples into left / right. Mod pointers are
        read from index modOffset onwards. */
    void render (float* left, float* right, int numSamples, const VoiceMod& mod, int modOffset) noexcept;

    int getLatencySamples() const noexcept { return downL.getLatencyLowRate(); }

    // State for the GUI and tests
    float getFilterEnv() const noexcept    { return filterEnv.getValue(); }
    float getAccentCharge() const noexcept { return accentCircuit.getCharge(); }
    float getCurrentPitch() const noexcept { return currentNote; }
    float getLastCutoffHz() const noexcept { return lastCutoffHz; }

private:
    float driveShape (float x) const noexcept;

    double sr = 44100.0, osr = 88200.0;
    VoiceSettings set;

    Oscillator unison[kMaxUnison], oscB;
    Ladder ladderL, ladderR;
    FilterEnvelope filterEnv, accentEnv;
    AmpEnvelope ampEnv;
    AccentCircuit accentCircuit;
    OnePole retroLpL, retroLpR;
    DcBlocker dcL, dcR;
    Halfband2x downL { 31 }, downR { 31 };
    FastRandom rng { 0x5eed1234u };

    float currentNote = 48.0f, targetNote = 48.0f, glideCoef = 1.0f;
    float noteDrift = 0.0f, retroDrift = 0.0f, retroDriftTarget = 0.0f;
    int retroCounter = 0;
    bool gate = false, accentNote = false;
    float lastCutoffHz = 1000.0f;
    float unisonDetune[kMaxUnison] {}, unisonGainL[kMaxUnison] {}, unisonGainR[kMaxUnison] {};
    float smLow = 0.0f, smPeak = 0.0f, smRes = 0.0f, smMorph = 0.0f, smoothCoef = 1.0f;
    bool smoothersPrimed = false;
    int activeVoices = 1;
    bool stereo = false;
};
} // namespace acid
