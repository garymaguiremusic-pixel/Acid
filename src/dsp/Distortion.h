#pragma once

#include "DspCommon.h"

namespace acid
{
struct DistortionSettings
{
    bool  on = false;
    int   mode = 0;          // 0 Saturate, 1 Foldback
    float drive = 0.5f;      // 0..1
    float distort = 0.7f;    // 0..1, dry / wet
};

/** Saturate / Foldback distortion, 2x oversampled. Loud and aggressive by design: the
    distorted signal is NOT levelled back down to the dry one, so Drive pushes it harder
    and louder, like driving a real pedal.
      Input: the lows are trimmed and the mids pushed before the curve, so the bass stays
             tight and the squelch is what distorts.
      Saturate: up to about +36 dB into a hard, asymmetric curve (even + odd harmonics).
      Foldback: up to about +32 dB into a wave folder; the waveform folds back on itself
                more and more as Drive rises.
      Output: a low-pass takes the fizz off the top, then a DC blocker.
    The dry path is delayed by the oversampling latency (also while off), so the plugin's
    latency never changes. Switching on / off crossfades. */
class Distortion
{
public:
    void prepare (double sampleRate);
    void reset();
    void setSettings (const DistortionSettings& s) noexcept;
    void process (float* left, float* right, int numSamples) noexcept;

    static int getLatencySamples() noexcept { return 15; }   // a 31-tap half-band up + down

    /** The input gain Drive gives (linear). */
    static float driveGain (int mode, float drive) noexcept;

    /** The shaping curve at the oversampled rate, after the input gain (for tests). */
    static float shape (float x, int mode, float drive) noexcept;

private:
    static float curve (float x, int mode) noexcept;

    double sr = 44100.0;
    DistortionSettings set;
    Halfband2x up[2] { Halfband2x (31), Halfband2x (31) }, down[2] { Halfband2x (31), Halfband2x (31) };
    Biquad lowTrim[2], midPush[2], post1[2], post2[2];
    DcBlocker dc[2];
    std::array<float, 32> dryBuf[2] {};
    int dryPos = 0;
    Smoother onFade, driveSm, mixSm;
};
} // namespace acid
