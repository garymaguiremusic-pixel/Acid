#pragma once

// Small building blocks shared by the DSP classes. Plain C++ (no JUCE), so the
// engine can be unit-tested on its own.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace acid
{
inline constexpr float kPi    = 3.14159265358979323846f;
inline constexpr float kTwoPi = 6.28318530717958647692f;

inline float clampf (float v, float lo, float hi) noexcept   { return std::min (hi, std::max (lo, v)); }
inline float lerp (float a, float b, float t) noexcept        { return a + (b - a) * t; }
inline float dbToGain (float db) noexcept                     { return std::pow (10.0f, db * 0.05f); }
inline float gainToDb (float g) noexcept                      { return 20.0f * std::log10 (std::max (1.0e-9f, g)); }
inline float midiToHz (float note) noexcept                   { return 440.0f * std::exp2 ((note - 69.0f) / 12.0f); }

/** Rational tanh approximation, exact at 0 and saturating to +-1 at |x| >= 3. */
inline float fastTanh (float x) noexcept
{
    x = clampf (x, -3.0f, 3.0f);
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

/** Coefficient for a one-pole smoother/decay with the given time constant. */
inline float onePoleCoef (float timeMs, double sampleRate) noexcept
{
    if (timeMs <= 0.0f) return 1.0f;
    return 1.0f - std::exp (-1.0f / (timeMs * 0.001f * (float) sampleRate));
}

inline void flushDenormal (float& v) noexcept
{
    if (std::abs (v) < 1.0e-15f) v = 0.0f;
}

//==============================================================================
/** Exponential parameter smoother (about 99 % there after the given time). */
class Smoother
{
public:
    void prepare (double sampleRate, float timeMs) noexcept { coef = onePoleCoef (timeMs / 4.6f, sampleRate); }
    void setTarget (float t) noexcept   { target = t; }
    void snap (float v) noexcept        { current = target = v; }
    float next() noexcept               { current += (target - current) * coef; return current; }
    float getCurrent() const noexcept   { return current; }
    float getTarget() const noexcept    { return target; }

private:
    float current = 0.0f, target = 0.0f, coef = 1.0f;
};

//==============================================================================
/** Topology-preserving one-pole filter (low-pass and high-pass outputs). */
class OnePole
{
public:
    void setCutoff (float hz, double sampleRate) noexcept
    {
        const float g = std::tan (kPi * clampf (hz, 1.0f, 0.49f * (float) sampleRate) / (float) sampleRate);
        G = g / (1.0f + g);
    }
    void reset() noexcept { s = 0.0f; }

    float lowpass (float x) noexcept
    {
        const float v = (x - s) * G;
        const float y = v + s;
        s = y + v;
        flushDenormal (s);
        return y;
    }
    float highpass (float x) noexcept { return x - lowpass (x); }

private:
    float G = 0.5f, s = 0.0f;
};

//==============================================================================
/** RBJ biquad, transposed direct form II. */
class Biquad
{
public:
    void reset() noexcept { z1 = z2 = 0.0f; }

    void setLowShelf (float hz, float gainDb, double sr)  { shelf (hz, gainDb, sr, false); }
    void setHighShelf (float hz, float gainDb, double sr) { shelf (hz, gainDb, sr, true); }

    void setBandpass (float hz, float q, double sr)   // constant 0 dB peak gain
    {
        const float w = kTwoPi * clampf (hz, 10.0f, 0.45f * (float) sr) / (float) sr;
        const float alpha = std::sin (w) / (2.0f * q), cw = std::cos (w);
        const float a0 = 1.0f + alpha;
        set (alpha / a0, 0.0f, -alpha / a0, -2.0f * cw / a0, (1.0f - alpha) / a0);
    }

    void setAllpass (float hz, float q, double sr)
    {
        const float w = kTwoPi * clampf (hz, 10.0f, 0.45f * (float) sr) / (float) sr;
        const float alpha = std::sin (w) / (2.0f * q), cw = std::cos (w);
        const float a0 = 1.0f + alpha;
        set ((1.0f - alpha) / a0, -2.0f * cw / a0, 1.0f, -2.0f * cw / a0, (1.0f - alpha) / a0);
    }

    void setLowpass (float hz, float q, double sr)
    {
        const float w = kTwoPi * clampf (hz, 10.0f, 0.45f * (float) sr) / (float) sr;
        const float alpha = std::sin (w) / (2.0f * q), cw = std::cos (w);
        const float a0 = 1.0f + alpha;
        set ((1.0f - cw) * 0.5f / a0, (1.0f - cw) / a0, (1.0f - cw) * 0.5f / a0, -2.0f * cw / a0, (1.0f - alpha) / a0);
    }

    void setHighpass (float hz, float q, double sr)
    {
        const float w = kTwoPi * clampf (hz, 10.0f, 0.45f * (float) sr) / (float) sr;
        const float alpha = std::sin (w) / (2.0f * q), cw = std::cos (w);
        const float a0 = 1.0f + alpha;
        set ((1.0f + cw) * 0.5f / a0, -(1.0f + cw) / a0, (1.0f + cw) * 0.5f / a0, -2.0f * cw / a0, (1.0f - alpha) / a0);
    }

    float process (float x) noexcept
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        flushDenormal (z1);
        flushDenormal (z2);
        return y;
    }

    /** Magnitude response at a frequency (for drawing and tests). */
    float magnitude (float hz, double sr) const noexcept
    {
        const double w = 2.0 * 3.141592653589793 * hz / sr;
        const double cr = std::cos (w), ci = -std::sin (w), c2r = std::cos (2 * w), c2i = -std::sin (2 * w);
        const double nr = b0 + b1 * cr + b2 * c2r, ni = b1 * ci + b2 * c2i;
        const double dr = 1.0 + a1 * cr + a2 * c2r, di = a1 * ci + a2 * c2i;
        return (float) std::sqrt ((nr * nr + ni * ni) / (dr * dr + di * di));
    }

private:
    void set (float nb0, float nb1, float nb2, float na1, float na2) noexcept
    {
        b0 = nb0; b1 = nb1; b2 = nb2; a1 = na1; a2 = na2;
    }

    void shelf (float hz, float gainDb, double sr, bool high)
    {
        const float A = std::pow (10.0f, gainDb / 40.0f);
        const float w = kTwoPi * clampf (hz, 10.0f, 0.45f * (float) sr) / (float) sr;
        const float cw = std::cos (w), sw = std::sin (w);
        const float alpha = sw / 2.0f * std::sqrt (2.0f);   // S = 1
        const float k = 2.0f * std::sqrt (A) * alpha;
        float nb0, nb1, nb2, a0, na1, na2;
        if (! high)
        {
            nb0 = A * ((A + 1) - (A - 1) * cw + k);
            nb1 = 2 * A * ((A - 1) - (A + 1) * cw);
            nb2 = A * ((A + 1) - (A - 1) * cw - k);
            a0  = (A + 1) + (A - 1) * cw + k;
            na1 = -2 * ((A - 1) + (A + 1) * cw);
            na2 = (A + 1) + (A - 1) * cw - k;
        }
        else
        {
            nb0 = A * ((A + 1) + (A - 1) * cw + k);
            nb1 = -2 * A * ((A - 1) + (A + 1) * cw);
            nb2 = A * ((A + 1) + (A - 1) * cw - k);
            a0  = (A + 1) - (A - 1) * cw + k;
            na1 = 2 * ((A - 1) - (A + 1) * cw);
            na2 = (A + 1) - (A - 1) * cw - k;
        }
        set (nb0 / a0, nb1 / a0, nb2 / a0, na1 / a0, na2 / a0);
    }

    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f, z1 = 0.0f, z2 = 0.0f;
};

//==============================================================================
/** First-order DC blocker (about 8 Hz). */
class DcBlocker
{
public:
    void prepare (double sampleRate) noexcept { R = 1.0f - kTwoPi * 8.0f / (float) sampleRate; reset(); }
    void reset() noexcept { x1 = y1 = 0.0f; }
    float process (float x) noexcept
    {
        const float y = x - x1 + R * y1;
        x1 = x;
        y1 = y;
        flushDenormal (y1);
        return y;
    }

private:
    float R = 0.999f, x1 = 0.0f, y1 = 0.0f;
};

//==============================================================================
/** Linear-phase half-band FIR for 2x up/down sampling. The latency of an up + down
    pair is (taps - 1) / 2 samples at the lower rate. */
class Halfband2x
{
public:
    static constexpr int kMaxTaps = 63;

    explicit Halfband2x (int numTaps = 31) { design (numTaps); }

    void design (int numTaps)
    {
        taps = std::min (kMaxTaps, numTaps | 1);
        const int M = (taps - 1) / 2;
        const double beta = 8.0;
        auto i0 = [] (double x)
        {
            double sum = 1.0, term = 1.0;
            for (int k = 1; k < 30; ++k) { term *= (x / (2.0 * k)) * (x / (2.0 * k)); sum += term; }
            return sum;
        };
        double total = 0.0;
        for (int n = 0; n < taps; ++n)
        {
            const double t = n - M;
            const double sinc = t == 0.0 ? 0.5 : std::sin (0.5 * 3.141592653589793 * t) / (3.141592653589793 * t);
            const double r = (double) (n - M) / (double) M;
            const double w = i0 (beta * std::sqrt (std::max (0.0, 1.0 - r * r))) / i0 (beta);
            h[(size_t) n] = (float) (sinc * w);
            total += sinc * w;
        }
        for (int n = 0; n < taps; ++n)
            h[(size_t) n] = (float) (h[(size_t) n] / total);   // unity DC gain
        reset();
    }

    void reset() noexcept { upBuf.fill (0.0f); downBuf.fill (0.0f); upPos = downPos = 0; }

    int getLatencyLowRate() const noexcept { return (taps - 1) / 2; }

    /** One low-rate sample in, two high-rate samples out. */
    void upsample (float x, float& y0, float& y1) noexcept
    {
        y0 = 2.0f * push (upBuf, upPos, x);
        y1 = 2.0f * push (upBuf, upPos, 0.0f);
    }

    /** Two high-rate samples in, one low-rate sample out. */
    float downsample (float a, float b) noexcept
    {
        // keep the even output so the latency is a whole number of low-rate samples
        const float y = push (downBuf, downPos, a);
        pushOnly (downBuf, downPos, b);
        return y;
    }

private:
    void pushOnly (std::array<float, 2 * kMaxTaps>& buf, int& pos, float x) noexcept
    {
        pos = (pos == 0 ? taps : pos) - 1;
        buf[(size_t) pos] = x;
        buf[(size_t) (pos + taps)] = x;
    }

    // Circular buffer stored twice so the convolution reads contiguous memory.
    float push (std::array<float, 2 * kMaxTaps>& buf, int& pos, float x) noexcept
    {
        pos = (pos == 0 ? taps : pos) - 1;
        buf[(size_t) pos] = x;
        buf[(size_t) (pos + taps)] = x;
        const float* p = buf.data() + pos;
        float sum = 0.0f;
        for (int n = 0; n < taps; ++n)
            sum += h[(size_t) n] * p[n];
        return sum;
    }

    std::array<float, kMaxTaps> h {};
    std::array<float, 2 * kMaxTaps> upBuf {}, downBuf {};
    int taps = 31, upPos = 0, downPos = 0;
};

//==============================================================================
/** Small deterministic random generator (audio-thread safe). */
class FastRandom
{
public:
    explicit FastRandom (uint32_t seed = 0x1234567u) : state (seed ? seed : 1u) {}
    void seed (uint32_t s) noexcept { state = s ? s : 1u; }
    uint32_t nextInt() noexcept { state ^= state << 13; state ^= state >> 17; state ^= state << 5; return state; }
    float nextFloat() noexcept { return (float) (nextInt() >> 8) / 16777216.0f; }   // 0..1
    float nextBipolar() noexcept { return nextFloat() * 2.0f - 1.0f; }

private:
    uint32_t state;
};
} // namespace acid
