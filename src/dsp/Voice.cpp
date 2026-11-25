#include "Voice.h"

namespace acid
{
void Voice::prepare (double sampleRate)
{
    sr = sampleRate;
    osr = sampleRate * 2.0;

    for (auto& o : unison) o.prepare (osr);
    oscB.prepare (osr);
    ladderL.prepare (osr);
    ladderR.prepare (osr);
    filterEnv.prepare (sr);
    accentEnv.prepare (sr);
    ampEnv.prepare (sr);
    accentCircuit.prepare (sr);
    retroLpL.setCutoff (9000.0f, osr);
    retroLpR.setCutoff (9000.0f, osr);
    dcL.prepare (osr);
    dcR.prepare (osr);
    smoothCoef = onePoleCoef (8.0f, sr);
    smoothersPrimed = false;
    setSettings (set);
    reset();
}

void Voice::reset()
{
    ladderL.reset();
    ladderR.reset();
    filterEnv.reset();
    accentEnv.reset();
    ampEnv.reset();
    accentCircuit.reset();
    retroLpL.reset();
    retroLpR.reset();
    dcL.reset();
    dcR.reset();
    downL.reset();
    downR.reset();
    gate = false;
    for (int i = 0; i < kMaxUnison; ++i)
        unison[i].resetPhase (0.13 * i);
}

void Voice::setSettings (const VoiceSettings& s)
{
    set = s;
    const auto model = s.transistor ? Ladder::Model::Transistor : Ladder::Model::Diode;
    ladderL.setModel (model);
    ladderR.setModel (model);
    ampEnv.setTimes (s.ampAttackMs, s.ampDecayMs, s.ampHold, s.ampReleaseMs);
    accentCircuit.setSweep (s.accSweep);
    glideCoef = onePoleCoef (std::max (1.0f, s.slideMs) / 3.0f, sr);   // ~95 % of the way after slideMs

    activeVoices = std::clamp (s.unisonVoices, 1, kMaxUnison);
    stereo = activeVoices > 1 && s.unisonSpread > 0.001f;
    const float norm = 1.0f / std::sqrt ((float) activeVoices);
    for (int i = 0; i < activeVoices; ++i)
    {
        const float pos = activeVoices > 1 ? 2.0f * (float) i / (float) (activeVoices - 1) - 1.0f : 0.0f;
        unisonDetune[i] = pos * s.unisonSpread * 25.0f;                       // cents
        const float pan = 0.5f + 0.5f * pos * s.unisonSpread;                 // 0 = left, 1 = right
        unisonGainL[i] = std::cos (pan * kPi * 0.5f) * norm * 1.41421356f;
        unisonGainR[i] = std::sin (pan * kPi * 0.5f) * norm * 1.41421356f;
    }
}

void Voice::noteOn (int midiNote, bool accent, bool legato)
{
    targetNote = (float) midiNote;
    accentNote = accent;

    if (accent)
        accentEnv.trigger (std::max (0.3f, set.envAttackMs), set.accDecayMs);

    if (legato && gate)
        return;   // slide: the pitch glides, the envelopes carry on

    currentNote = targetNote;
    filterEnv.trigger (set.envAttackMs, accent ? set.accDecayMs : set.decayMs);
    ampEnv.gateOn();
    gate = true;

    if (set.retro)
        noteDrift = rng.nextBipolar() * 1.5f;     // cents
    else
        noteDrift = 0.0f;

    if (! set.unisonFree)
    {
        for (int i = 0; i < kMaxUnison; ++i)
            unison[i].resetPhase (0.13 * i);
        oscB.resetPhase (0.0);
    }
}

void Voice::noteOff()
{
    gate = false;
    ampEnv.gateOff();
}

float Voice::driveShape (float x) const noexcept
{
    const float d = set.drive;
    if (d <= 0.0f)
        return x;
    const float g = 1.0f + 14.0f * d * d;
    constexpr float bias = 0.18f;
    const float shaped = (fastTanh (g * x + bias) - fastTanh (bias)) / (1.0f + 0.25f * g);
    return x + (shaped * (1.0f + 0.25f * g) * 0.75f - x) * std::min (1.0f, d * 3.0f);
}

void Voice::render (float* left, float* right, int numSamples, const VoiceMod& mod, int modOffset) noexcept
{
    const float logLowTarget  = std::log2 (std::max (20.0f, std::min (set.cutoffLowHz, set.cutoffHz)));
    const float logPeakTarget = std::log2 (std::max (20.0f, set.cutoffHz)) + set.envMod * kEnvModOctaves;
    if (! smoothersPrimed)
    {
        smLow = logLowTarget; smPeak = logPeakTarget; smRes = set.resonance; smMorph = set.waveMorph;
        smoothersPrimed = true;
    }
    const float maxCutoff = std::min (22000.0f, 0.42f * (float) osr);

    for (int n = 0; n < numSamples; ++n)
    {
        const int m = n + modOffset;

        // smooth the knob-driven targets (no zipper on whole-step knob moves)
        smLow   += (logLowTarget - smLow) * smoothCoef;
        smPeak  += (logPeakTarget - smPeak) * smoothCoef;
        smRes   += (set.resonance - smRes) * smoothCoef;
        smMorph += (set.waveMorph - smMorph) * smoothCoef;
        const float logLow = smLow, logPeak = smPeak;

        // --- pitch -------------------------------------------------------------------
        currentNote += (targetNote - currentNote) * glideCoef;

        if (set.retro)
        {
            if (--retroCounter <= 0)
            {
                retroCounter = (int) (sr * 0.05);
                retroDriftTarget = rng.nextBipolar() * 3.0f;     // +-3 cents
            }
            retroDrift += (retroDriftTarget - retroDrift) * 0.0002f;
        }
        else
        {
            retroDrift = 0.0f;
        }

        const float pitch = currentNote + set.tuneSemis + (mod.pitchSemis ? mod.pitchSemis[m] : 0.0f)
                          + (retroDrift + noteDrift) * 0.01f;
        const float baseHz = midiToHz (pitch);
        for (int i = 0; i < activeVoices; ++i)
            unison[i].setFrequency (baseHz * std::exp2 (unisonDetune[i] / 1200.0f));
        if (set.oscBLevel > 0.0f)
            oscB.setFrequency (baseHz * std::exp2 ((set.oscBTuneSemis + set.oscBDetuneCents * 0.01f) / 12.0f));

        // --- envelopes, accent and cutoff ----------------------------------------------
        const float fEnv = filterEnv.process();
        const float aEnv = accentEnv.process();
        const float accentPulse = aEnv * set.accent;
        const float accentCv = accentCircuit.process (accentPulse);
        const float amp = ampEnv.process();

        float logF = logLow + fEnv * (logPeak - logLow) + accentCv * kAccentOctaves;
        if (mod.cutoffOct) logF += mod.cutoffOct[m];
        const float cutoff = clampf (std::exp2 (logF), 20.0f, maxCutoff);
        lastCutoffHz = cutoff;

        const float reso = clampf (smRes + (mod.resonance ? mod.resonance[m] : 0.0f), 0.0f, 1.0f);
        const float morph = clampf (smMorph + (mod.wave ? mod.wave[m] : 0.0f), 0.0f, 1.0f);
        float vca = amp * (1.0f + 0.9f * accentPulse);
        if (mod.volume) vca *= mod.volume[m];

        // --- oversampled audio ---------------------------------------------------------
        float outL[2], outR[2];
        for (int k = 0; k < 2; ++k)
        {
            float l = 0.0f, r = 0.0f;
            for (int i = 0; i < activeVoices; ++i)
            {
                const float s = unison[i].process (morph);
                l += s * unisonGainL[i];
                r += s * unisonGainR[i];
            }
            if (set.oscBLevel > 0.0f)
            {
                const float b = oscB.process (morph) * set.oscBLevel;
                l += b;
                r += b;
            }

            if (set.retro)
            {
                l = retroLpL.lowpass (l);
                r = retroLpR.lowpass (r);
            }

            l = ladderL.process (l * 0.7f, cutoff, reso);
            r = stereo ? ladderR.process (r * 0.7f, cutoff, reso) : l;

            l = dcL.process (driveShape (l * vca));
            r = stereo ? dcR.process (driveShape (r * vca)) : l;
            outL[k] = l;
            outR[k] = r;
        }

        left[n] = kOutputTrim * downL.downsample (outL[0], outL[1]);
        right[n] = stereo ? kOutputTrim * downR.downsample (outR[0], outR[1]) : left[n];
        if (! stereo)
            downR.downsample (outL[0], outL[1]);   // keep its history in step for a clean switch
    }
}
} // namespace acid
