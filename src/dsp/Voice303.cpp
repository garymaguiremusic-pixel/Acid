#include "Voice303.h"

namespace acid
{
VoiceSettings Voice303::mapSettings (const Settings303& s)
{
    VoiceSettings v;
    v.tuneSemis = s.tuningSemis + 12.0f * (float) std::clamp (s.octave, -2, 2);
    v.waveMorph = s.wave != 0 ? 1.0f : 0.0f;
    v.retro = s.model != Settings303::Stable;          // Classic / Drift: the analog wander

    // cutoff: ~300 Hz .. 2.7 kHz, pulled down by Env Mod (up to about an octave)
    const float envMod = clampf (s.envMod, 0.0f, 1.0f);
    const float base = 300.0f * std::exp2 (clampf (s.cutoff, 0.0f, 1.0f) * 3.2f) * std::exp2 (-envMod * 1.1f);
    v.cutoffHz = base;
    v.cutoffLowHz = base;                              // the envelope falls back to the base cutoff
    v.envMod = 0.12f + envMod * 0.5f;                  // up to ~3 octaves of sweep above the base
    v.resonance = clampf (s.resonance, 0.0f, 1.0f) * 0.97f;
    v.transistor = false;                              // the diode ladder
    v.filterMode = 0;

    // envelopes: Decay for normal notes, the shortest decay for accented ones
    v.decayMs = 200.0f * std::pow (10.0f, clampf (s.decay, 0.0f, 1.0f));
    v.accDecayMs = 200.0f;
    v.envAttackMs = 3.0f;

    // accent: amount, and a sweep that builds more at high resonance
    v.accent = clampf (s.accent, 0.0f, 1.0f) * (s.softAccent ? 0.6f : 1.0f);
    v.accSweep = clampf (0.25f + 0.55f * s.resonance + (s.weakAccentCap ? 0.3f : 0.0f), 0.0f, 1.0f);
    v.drive = 0.0f;

    // VCA: quick attack, slow decay while the gate is held, a fast close
    v.ampAttackMs = s.accurateGate ? 1.5f : 0.5f;
    v.ampDecayMs = 3200.0f;
    v.ampHold = s.infSustain;
    v.ampReleaseMs = s.accurateGate ? 16.0f : 8.0f;

    v.slideMs = s.slideMs * (s.slowSlide ? 2.0f : 1.0f);
    v.oscALock = s.phaseReset;
    v.oscBLevel = clampf (s.oscBLevel, 0.0f, 1.0f);
    v.oscBTuneSemis = s.oscBTuneSemis;
    v.oscBDetuneCents = s.oscBDetuneCents;
    v.oscBLock = s.oscBLock;
    v.unisonVoices = std::clamp (s.unisonVoices, 1, Voice::kMaxUnison);
    v.unisonSpread = clampf (s.unisonSpread, 0.0f, 1.0f);
    v.unisonFree = s.unisonFree;
    return v;
}

void Voice303::prepare (double sampleRate, int maxBlock)
{
    sr = sampleRate;
    pitchMod.assign ((size_t) std::max (1, maxBlock), 0.0f);
    voice.prepare (sampleRate);
    setSettings (set);
}

void Voice303::reset()
{
    voice.reset();
    vibPhase = driftPhase = 0.0;
    drift = driftTarget = 0.0f;
}

void Voice303::setSettings (const Settings303& s)
{
    set = s;
    voice.setSettings (mapSettings (s));
}

void Voice303::noteOn (int midiNote, bool accent, bool legato, float level, bool glide)
{
    if (! legato || ! voice.isGateOn())
        vibPhase = 0.0;
    voice.noteOn (midiNote, accent, legato, level, 1.0f, glide);
}

void Voice303::render (float* left, float* right, int numSamples, const VoiceMod& lfo) noexcept
{
    const bool vib = set.vibrato, wander = set.model == Settings303::Drift;
    int pos = 0;
    while (pos < numSamples)
    {
        const int n = std::min (numSamples - pos, (int) pitchMod.size());
        auto offset = [pos] (const float* p) { return p != nullptr ? p + pos : nullptr; };
        VoiceMod mod;
        mod.cutoffOct = offset (lfo.cutoffOct);
        mod.wave      = offset (lfo.wave);
        mod.resonance = offset (lfo.resonance);
        mod.volume    = offset (lfo.volume);
        mod.pitchSemis = offset (lfo.pitchSemis);
        if (vib || wander)
        {
            for (int i = 0; i < n; ++i)
            {
                float p = lfo.pitchSemis != nullptr ? lfo.pitchSemis[pos + i] : 0.0f;
                if (vib)
                {
                    vibPhase += 5.5 / sr;
                    if (vibPhase >= 1.0) vibPhase -= 1.0;
                    p += 0.15f * std::sin (kTwoPi * (float) vibPhase);          // +-15 cents
                }
                if (wander)
                {
                    if (--driftCounter <= 0)
                    {
                        driftCounter = (int) (sr * 0.25);
                        driftTarget = rng.nextBipolar() * 0.08f;                    // +-8 cents
                    }
                    drift += (driftTarget - drift) * 0.00005f;
                    p += drift;
                }
                pitchMod[(size_t) i] = p;
            }
            mod.pitchSemis = pitchMod.data();
        }

        voice.render (left + pos, right + pos, n, mod, 0);

        const float gain = 1.4f * set.volume * set.volume;
        for (int i = 0; i < n; ++i)
        {
            float l = left[pos + i] * gain, r = right[pos + i] * gain;
            if (set.vcaNoise)
            {
                const float hiss = rng.nextBipolar() * 0.00012f;                // about -78 dBFS
                l += hiss;
                r += hiss;
            }
            left[pos + i] = l;
            right[pos + i] = r;
        }
        pos += n;
    }
}
} // namespace acid
