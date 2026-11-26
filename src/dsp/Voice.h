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
    int   filterMode      = 0;        // 0 LP, 1 BP, 2 HP
    bool  oscALock        = false;    // phase lock: Osc A (and its unison voices) restart on every note
    bool  oscBLock        = false;    // phase lock: Osc B restarts on every note
    bool  velLink         = false;    // velocity moves the cutoff
    float velAmount       = 0.5f;     // 0..1: how far (up to kVelocityOctaves)

    bool  oscSync         = false;    // hard sync: Osc 1 is locked to the note and swept up by syncSemis
    float syncSemis       = 12.0f;    // how far Osc 1 sits above the note while synced
    float fmAmount        = 0.0f;     // 0..1
    bool  fmBtoA          = false;    // false: Osc 1 drives Osc 2 (1 > 2), true: Osc 2 drives Osc 1 (2 > 1)
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

    /** De-click: the VCA's on / off edges are never brighter than the filter lets through
        (the lower the cutoff, the softer the edge). On for the Acid synth. */
    bool  smoothGate      = false;
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
    static constexpr float kVelocityOctaves = 4.0f;   // Vel Link at 100 %: the lowest velocity closes this far
    static constexpr int kBaseNote = 36;          // C2: the lowest note of a pattern at octave 0
    static constexpr float kOutputTrim = 0.9f;
    static constexpr float kFmDepth = 1.5f;       // FM at 100 %: the phase moves up to 1.5 cycles    // output level (headroom for the tube and echo)

    void prepare (double sampleRate);
    void reset();
    void setSettings (const VoiceSettings& s);

    /** legato: no retrigger of the envelopes; the pitch glides there over the slide time
        (glide = true), or moves straight to it (glide = false, a plain tie).
        level (0..1) scales the note's loudness (the sequencer's velocity x Volume).
        filterVelocity (0..1) drives Vel Link: 1 leaves the cutoff, lower closes it. */
    void noteOn (int midiNote, bool accent, bool legato, float level = 1.0f, float filterVelocity = 1.0f,
                 bool glide = true);
    void noteOff();
    bool isGateOn() const noexcept { return gate; }
    /** True while the voice still makes sound (gate on, or the release still running). */
    bool isSounding() const noexcept { return ampEnv.isActive(); }

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
    float levelTarget = 1.0f, levelSm = 1.0f;
    float velTarget = 1.0f, velSm = 1.0f;
    float vcaSm[2] {};
    double syncPhase[kMaxUnison] {}, syncInc[kMaxUnison] {};   // hard sync: the hidden masters at the note's pitch
    int activeVoices = 1;
    bool stereo = false;
};
} // namespace acid
