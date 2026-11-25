#pragma once

#include "Voice.h"

namespace acid
{
/** The 303 page's controls, already in real units. */
struct Settings303
{
    int   wave        = 0;       // 0 saw, 1 square
    float tuningSemis = 0.0f;    // -12..+12
    int   octave      = 0;       // -2..+2
    float cutoff      = 0.5f;    // 0..1 (the knob)
    float resonance   = 0.7f;    // 0..1
    float envMod      = 0.5f;    // 0..1
    float decay       = 0.4f;    // 0..1
    float accent      = 0.6f;    // 0..1
    float volume      = 0.7f;    // 0..1
    float slideMs     = 60.0f;   // the sequencer's Glide

    enum Model { Classic = 0, Stable, Drift };
    int  model          = Classic;
    bool accurateGate   = false;   // softer, slightly longer gate edges, like the hardware's VCA
    bool vcaNoise       = true;    // the faint hiss of the hardware's VCA
    bool vibrato        = false;   // a gentle 5.5 Hz vibrato
    bool phaseReset     = false;   // the oscillator restarts on every note
    bool slowSlide      = false;   // an aged slide capacitor: twice the glide time
    bool weakAccentCap  = false;   // an aged accent capacitor: more accent build-up
    bool infSustain     = false;   // the VCA holds while the gate is open (no decay)
    bool softAccent     = false;   // a gentler accent
};

/** A 303-style voice, built on Acid's diode-ladder voice with the hardware's behaviour:
      - the cutoff knob covers about 300 Hz - 2.7 kHz, and Env Mod also pulls the base
        cutoff down as it rises (as on the hardware)
      - Decay sets the filter envelope of normal notes (about 200 ms - 2 s); accented notes
        always use the shortest decay
      - accent: louder and brighter, and the accent sweep builds up more at high resonance
      - the VCA decays slowly while the gate is held and closes fast when it ends
    The design is our own; it is not a copy of any other plugin. */
class Voice303
{
public:
    void prepare (double sampleRate, int maxBlock);
    void reset();
    void setSettings (const Settings303&);

    void noteOn (int midiNote, bool accent, bool legato, float level = 1.0f, bool glide = true);
    void noteOff()                              { voice.noteOff(); }
    bool isGateOn() const noexcept              { return voice.isGateOn(); }
    bool isSounding() const noexcept            { return voice.isSounding(); }

    /** Overwrites left / right with numSamples of output. */
    void render (float* left, float* right, int numSamples) noexcept;

    int getLatencySamples() const noexcept      { return voice.getLatencySamples(); }
    float getFilterEnv() const noexcept         { return voice.getFilterEnv(); }
    float getAccentCharge() const noexcept      { return voice.getAccentCharge(); }
    float getCurrentPitch() const noexcept      { return voice.getCurrentPitch(); }
    float getLastCutoffHz() const noexcept      { return voice.getLastCutoffHz(); }

    /** The voice settings these controls map to (for tests). */
    static VoiceSettings mapSettings (const Settings303&);

private:
    Voice voice;
    Settings303 set;
    double sr = 44100.0;
    std::vector<float> pitchMod;
    double vibPhase = 0.0, driftPhase = 0.0;
    float driftTarget = 0.0f, drift = 0.0f;
    int driftCounter = 0;
    FastRandom rng { 0x303u };
};
} // namespace acid
