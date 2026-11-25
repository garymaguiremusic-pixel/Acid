#pragma once

#include "DspCommon.h"
#include "Crossover.h"

namespace acid
{
struct TubeSettings
{
    float edges[4]     = { 62.0f, 748.0f, 2100.0f, 11000.0f };   // low edge, x-over 1, x-over 2, high edge
    float drive[3]     = { 0.3f, 0.3f, 0.3f };                     // 0..1 per band
    float bandGainDb[3]= { 0.0f, 0.0f, 0.0f };
    bool  solo[3]      = { false, false, false };
    float resonator    = 0.0f;   // 0..1
    float character    = 0.3f;   // 0 = symmetric (odd), 1 = asymmetric (even)
    float inputDb      = 0.0f;
    float outputDb     = 0.0f;
    float mix          = 1.0f;   // 0..1
    bool  active       = true;
};

/** Three-band tube distortion. Linkwitz-Riley crossovers split the signal; content
    below the low edge and above the high edge passes clean. Each band gets a resonant
    emphasis, then a 4x oversampled tube stage (biased tanh, DC blocked) and a gain.
    The clean parts and the dry signal are delayed to match the oversampling latency,
    which is constant (also while bypassed) so the host can compensate for it. */
class MultibandTube
{
public:
    static constexpr int kStage1Taps = 31, kStage2Taps = 17;

    void prepare (double sampleRate);
    void reset();
    void setSettings (const TubeSettings&);
    void process (float* left, float* right, int numSamples) noexcept;

    static int getLatencySamples() noexcept { return (kStage1Taps - 1) / 2 + (kStage2Taps - 1) / 4; }

    /** Per-band output level (0..1-ish, smoothed) for the tube glow. */
    float getBandLevel (int band) const noexcept { return bandLevel[band]; }

    /** The tube transfer curve (also used by the GUI). */
    static float tubeCurve (float x, float drive01, float character01) noexcept;

private:
    struct Band
    {
        Biquad resonator;
        Halfband2x up1 { kStage1Taps }, up2 { kStage2Taps };
        Halfband2x down1 { kStage1Taps }, down2 { kStage2Taps };
        DcBlocker dc;
        float process (float x, float drive, float character, float resAmount) noexcept;
        void reset();
    };

    struct DelayLine
    {
        std::array<float, 64> buf {};
        int pos = 0;
        float process (float x, int delay) noexcept
        {
            buf[(size_t) pos] = x;
            const int r = (pos - delay + 64) & 63;
            pos = (pos + 1) & 63;
            return buf[(size_t) r];
        }
        void reset() { buf.fill (0.0f); pos = 0; }
    };

    double sr = 44100.0;
    TubeSettings set;
    FiveWaySplit split[2];
    Band bands[2][3];
    DelayLine cleanDelay[2], dryDelay[2];
    Smoother activeFade, mixSmooth, inGain, outGain, bandGain[3];
    float bandLevel[3] {}, levelCoef = 0.001f;
    float bandMakeup[3] { 1.0f, 1.0f, 1.0f };
    float lastEdges[4] {};
};
} // namespace acid
