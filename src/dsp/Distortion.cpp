#include "Distortion.h"

namespace acid
{
float Distortion::driveGain (int mode, float drive) noexcept
{
    drive = clampf (drive, 0.0f, 1.0f);
    // Saturate: +6 .. +36 dB, Foldback: +4 .. +32 dB
    const float db = mode == 1 ? 4.0f + 28.0f * drive : 6.0f + 30.0f * drive;
    return std::pow (10.0f, db / 20.0f);
}

float Distortion::curve (float x, int mode) noexcept
{
    if (mode == 1)
    {
        // wave folder: a sine fold, rounded off so the peaks stay musical
        const float folded = std::sin (x * 1.5707963f);
        return 0.95f * folded;
    }

    // saturate: asymmetric (the positive side clips harder), with a hard knee
    const float bias = 0.18f;
    const float y = fastTanh (1.4f * (x + bias)) - fastTanh (1.4f * bias);
    return clampf (y * 1.1f, -0.98f, 0.92f);
}

float Distortion::shape (float x, int mode, float drive) noexcept
{
    return curve (x * driveGain (mode, drive), mode);
}

void Distortion::prepare (double sampleRate)
{
    sr = sampleRate;
    for (int c = 0; c < 2; ++c)
    {
        lowTrim[c].setLowShelf (180.0f, -9.0f, sr);       // keep the bass tight
        midPush[c].setBandpass (1100.0f, 0.7f, sr);       // added on top: the squelch drives hardest
        post1[c].setLowpass (7500.0f, 0.6f, sr);          // take the fizz off
        post2[c].setLowpass (11000.0f, 0.5f, sr);
        dc[c].prepare (sr);
    }
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
        lowTrim[c].reset();
        midPush[c].reset();
        post1[c].reset();
        post2[c].reset();
        dc[c].reset();
        dryBuf[c].fill (0.0f);
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
    const int mode = set.mode;
    for (int i = 0; i < numSamples; ++i)
    {
        const float fade = onFade.next(), drive = driveSm.next(), mix = mixSm.next();
        const float gain = driveGain (mode, drive);
        const int readPos = (dryPos - latency + 32) & 31;

        for (int c = 0; c < 2; ++c)
        {
            const float x = io[c][i];
            dryBuf[c][(size_t) dryPos] = x;
            const float dry = dryBuf[c][(size_t) readPos];

            // input voicing, then the gain into the curve
            float pre = lowTrim[c].process (x);
            pre += 1.2f * midPush[c].process (pre);
            pre *= gain;

            float a, b;
            up[c].upsample (pre, a, b);
            a = curve (a, mode);
            b = curve (b, mode);
            float wet = down[c].downsample (a, b);
            wet = dc[c].process (post2[c].process (post1[c].process (wet)));

            const float processed = dry + (wet - dry) * mix;
            io[c][i] = dry + (processed - dry) * fade;
        }
        dryPos = (dryPos + 1) & 31;
    }
}
} // namespace acid
