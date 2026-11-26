#pragma once

#include "DspCommon.h"

namespace acid
{
/** Band-limited saw (PolyBLEP) with a 303-style square derived from it through a
    biased tanh waveshaper (rounded edges and a tilted top, not a perfect square).
    morph 0 = saw, 1 = square, anything between crossfades. */
class Oscillator
{
public:
    void prepare (double sampleRate) noexcept  { sr = sampleRate; }
    void resetPhase (double phase) noexcept     { ph = phase - std::floor (phase); }
    void setFrequency (float hz) noexcept       { inc = std::min (0.45, (double) std::max (0.0f, hz) / sr); }
    double getPhase() const noexcept            { return ph; }

    float process (float morph) noexcept;

    /** process() with the phase pushed by 'pm' cycles (FM, as phase modulation). */
    float process (float morph, float pm) noexcept;

    /** True if the last process() call wrapped; wrapFraction() is how far past the wrap
        it went, as a fraction of one sample (for hard sync). */
    bool justWrapped() const noexcept           { return wrapped; }
    double wrapFraction() const noexcept        { return inc > 0.0 ? ph / inc : 0.0; }

    /** Hard sync: restart the cycle, 'fraction' of a sample after the master wrapped. */
    void syncReset (double fraction) noexcept   { ph = std::clamp (fraction, 0.0, 1.0) * inc; }

    /** The same waveform without band-limiting, for drawing (phase 0..1). */
    static float shapeAt (float phase, float morph) noexcept;

private:
    static float squareFromSaw (float saw) noexcept;
    double sr = 44100.0, ph = 0.0, inc = 0.0;
    bool wrapped = false;
};
} // namespace acid
