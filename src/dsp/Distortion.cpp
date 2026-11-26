#include "Distortion.h"

namespace acid
{
float Distortion::driveGain (int mode, float drive) noexcept
{
    drive = clampf (drive, 0.0f, 1.0f);
    // dB at no drive and at full drive, per mode
    static constexpr float lo[] = { 6.0f, 4.0f, 8.0f, 12.0f, 4.0f, 4.0f };
    static constexpr float hi[] = { 36.0f, 32.0f, 38.0f, 42.0f, 30.0f, 30.0f };
    const int m = std::clamp (mode, 0, 5);
    const float db = lo[m] + (hi[m] - lo[m]) * drive;
    return std::pow (10.0f, db / 20.0f);
}

float Distortion::curve (float x, int mode) noexcept
{
    switch (mode)
    {
        case 1:     // foldback: a sine fold, rounded off so the peaks stay musical
            return 0.95f * std::sin (x * 1.5707963f);

        case 2:     // overdrive: a hard clip with a sharp knee, the negative side clipping later
        {
            const float k = x >= 0.0f ? x : x * 0.8f;
            const float a = std::abs (k);
            const float y = k / std::pow (1.0f + a * a * a * a * a * a, 1.0f / 6.0f);
            return 0.95f * (x >= 0.0f ? y : y * 1.25f);
        }

        case 3:     // fuzz: a dead zone (it sputters as notes fade), then a steep, square-edged curve
        {
            const float a = std::max (0.0f, std::abs (x) - 0.35f);
            const float y = fastTanh (3.0f * a);
            return 0.92f * (x >= 0.0f ? y : -0.9f * y);
        }

        case 4:     // scream: a plain hard clip (the resonant peak goes in before it)
            return 0.92f * clampf (x, -1.0f, 1.0f);

        case 5:     // rectify: mostly the rectified wave (octave up), a little of the clipped wave
        {
            const float s = fastTanh (x);
            return 0.95f * clampf (1.5f * std::abs (s) - 0.75f + 0.35f * s, -1.0f, 1.0f);
        }

        default:
            break;
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
        screamPeak[c].setBandpass (1600.0f, 3.0f, sr);    // Scream: a narrow, resonant peak
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
        screamPeak[c].reset();
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
    const int modes = activeModes();
    int first = 0;
    while (first < 6 && (modes & (1 << first)) == 0) ++first;
    const bool scream = (modes & (1 << 4)) != 0;
    for (int i = 0; i < numSamples; ++i)
    {
        const float fade = onFade.next(), drive = driveSm.next(), mix = mixSm.next();
        const float gain = first < 6 ? driveGain (first, drive) : 1.0f;
        const float between = 1.5f + 2.5f * drive;           // into each further mode
        const int readPos = (dryPos - latency + 32) & 31;

        for (int c = 0; c < 2; ++c)
        {
            const float x = io[c][i];
            dryBuf[c][(size_t) dryPos] = x;
            const float dry = dryBuf[c][(size_t) readPos];

            // input voicing, then the gain into the curve
            float pre = lowTrim[c].process (x);
            pre += 1.2f * midPush[c].process (pre);
            if (scream)
                pre += 4.0f * screamPeak[c].process (pre);
            pre *= gain;

            float a, b;
            up[c].upsample (pre, a, b);
            // the modes that are on, one after another (none on: clean)
            bool firstStage = true;
            for (int m = 0; m < 6; ++m)
            {
                if ((modes & (1 << m)) == 0)
                    continue;
                if (! firstStage) { a *= between; b *= between; }
                a = curve (a, m);
                b = curve (b, m);
                firstStage = false;
            }
            float wet = down[c].downsample (a, b);
            wet = dc[c].process (post2[c].process (post1[c].process (wet)));
            if (first >= 6)
                wet = dry;                                     // no mode on: clean

            const float processed = dry + (wet - dry) * mix;
            io[c][i] = dry + (processed - dry) * fade;
        }
        dryPos = (dryPos + 1) & 31;
    }
}
} // namespace acid
