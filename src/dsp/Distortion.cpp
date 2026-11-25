#include "Distortion.h"

namespace acid
{
float Distortion::shape (float x, int mode, float drive) noexcept
{
    drive = clampf (drive, 0.0f, 1.0f);
    if (mode == 1)
    {
        // foldback: barely folds at no drive, folds many times at full
        const float k = 1.5f + drive * 28.0f;
        return 0.7f * std::sin (x * k);
    }
    // saturate: the gain rises steeply with drive (the level is matched afterwards)
    const float g = 1.0f + 60.0f * drive * drive;
    return fastTanh (x * g);
}

void Distortion::prepare (double sampleRate)
{
    sr = sampleRate;
    for (auto& s : smooth) s.setCutoff (9000.0f, sr * 2.0);
    for (auto& d : dc) d.prepare (sr);
    envCoef = onePoleCoef (300.0f, sr);
    gainCoef = onePoleCoef (80.0f, sr);
    onFade.prepare (sr, 20.0f);
    driveSm.prepare (sr, 30.0f);
    mixSm.prepare (sr, 30.0f);
    onFade.snap (set.on ? 1.0f : 0.0f);
    driveSm.snap (set.drive);
    mixSm.snap (set.distort);
    reset();
}

void Distortion::reset()
{
    for (int c = 0; c < 2; ++c)
    {
        up[c].reset();
        down[c].reset();
        smooth[c].reset();
        dc[c].reset();
        dryBuf[c].fill (0.0f);
        dryEnv[c] = wetEnv[c] = 0.0f;
        makeup[c] = 1.0f;
    }
    dryPos = 0;
}

void Distortion::setSettings (const DistortionSettings& s) noexcept
{
    set = s;
    onFade.setTarget (s.on ? 1.0f : 0.0f);
    driveSm.setTarget (clampf (s.drive, 0.0f, 1.0f));
    mixSm.setTarget (clampf (s.distort, 0.0f, 1.0f));
}

void Distortion::process (float* left, float* right, int numSamples) noexcept
{
    float* io[2] = { left, right };
    const int latency = getLatencySamples();
    for (int i = 0; i < numSamples; ++i)
    {
        const float fade = onFade.next(), drive = driveSm.next(), mix = mixSm.next();
        const int readPos = (dryPos - latency + 32) & 31;

        for (int c = 0; c < 2; ++c)
        {
            const float x = io[c][i];
            dryBuf[c][(size_t) dryPos] = x;
            const float dry = dryBuf[c][(size_t) readPos];

            float a, b;
            up[c].upsample (x, a, b);
            a = shape (smooth[c].lowpass (a), set.mode, drive);
            b = shape (smooth[c].lowpass (b), set.mode, drive);
            float wet = dc[c].process (down[c].downsample (a, b));

            // level matching: the distorted signal follows the loudness of the dry one
            // so more drive means more grit rather than more volume
            dryEnv[c] += (dry * dry - dryEnv[c]) * envCoef;
            wetEnv[c] += (wet * wet - wetEnv[c]) * envCoef;
            const float target = clampf (1.0f * std::sqrt ((dryEnv[c] + 1.0e-9f) / (wetEnv[c] + 1.0e-9f)), 0.05f, 4.0f);
            makeup[c] += (target - makeup[c]) * gainCoef;
            wet *= makeup[c];

            const float processed = dry + (wet - dry) * mix;
            io[c][i] = dry + (processed - dry) * fade;
        }
        dryPos = (dryPos + 1) & 31;
    }
}
} // namespace acid
