#pragma once

#include "DspCommon.h"

namespace acid
{
/** Post-filter Bass / Treble shelves (+-12 dB), stereo. */
class ToneStack
{
public:
    static constexpr float kBassHz = 110.0f, kTrebleHz = 3500.0f;

    void prepare (double sampleRate) noexcept { sr = sampleRate; setGains (bass, treble, true); reset(); }
    void reset() noexcept { for (auto* f : { &lowL, &lowR, &highL, &highR }) f->reset(); }

    void setGains (float bassDb, float trebleDb, bool force = false) noexcept
    {
        if (! force && bassDb == bass && trebleDb == treble)
            return;
        bass = bassDb;
        treble = trebleDb;
        lowL.setLowShelf (kBassHz, bass, sr);
        lowR.setLowShelf (kBassHz, bass, sr);
        highL.setHighShelf (kTrebleHz, treble, sr);
        highR.setHighShelf (kTrebleHz, treble, sr);
    }

    void process (float* left, float* right, int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
        {
            left[i] = highL.process (lowL.process (left[i]));
            right[i] = highR.process (lowR.process (right[i]));
        }
    }

private:
    double sr = 44100.0;
    float bass = 0.0f, treble = 0.0f;
    Biquad lowL, lowR, highL, highR;
};
} // namespace acid
