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

/** Saturate / Foldback distortion, 2x oversampled.
      Saturate: a gently pre-smoothed signal through a tanh curve; more drive = harder
                clipping. The distorted signal's level follows the dry signal's, so Drive changes the grit rather than the volume.
      Foldback: the waveform folds back on itself (a sine fold); drive sets how often.
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

    /** The shaping curve at the oversampled rate (for tests). */
    static float shape (float x, int mode, float drive) noexcept;

private:
    double sr = 44100.0;
    DistortionSettings set;
    Halfband2x up[2] { Halfband2x (31), Halfband2x (31) }, down[2] { Halfband2x (31), Halfband2x (31) };
    OnePole smooth[2];
    DcBlocker dc[2];
    std::array<float, 32> dryBuf[2] {};
    int dryPos = 0;
    float dryEnv[2] {}, wetEnv[2] {}, makeup[2] { 1.0f, 1.0f }, envCoef = 0.001f, gainCoef = 0.001f;
    Smoother onFade, driveSm, mixSm;
};
} // namespace acid
