#pragma once

#include "DspCommon.h"

namespace acid
{
/** Linkwitz-Riley 4th-order crossover (two cascaded Butterworth sections). The low
    and high outputs are in phase and sum to a 2nd-order all-pass, so the bands add
    back up flat. */
class LR4Crossover
{
public:
    void setFrequency (float hz, double sr) noexcept
    {
        constexpr float q = 0.70710678f;
        lp1.setLowpass (hz, q, sr);  lp2.setLowpass (hz, q, sr);
        hp1.setHighpass (hz, q, sr); hp2.setHighpass (hz, q, sr);
    }
    void reset() noexcept { lp1.reset(); lp2.reset(); hp1.reset(); hp2.reset(); }

    void split (float x, float& low, float& high) noexcept
    {
        low = lp2.process (lp1.process (x));
        high = hp2.process (hp1.process (x));
    }

private:
    Biquad lp1, lp2, hp1, hp2;
};

/** The all-pass an LR4 crossover applies to the sum (for phase-aligning other bands). */
class LR4Allpass
{
public:
    void setFrequency (float hz, double sr) noexcept { ap.setAllpass (hz, 0.70710678f, sr); }
    void reset() noexcept { ap.reset(); }
    float process (float x) noexcept { return ap.process (x); }

private:
    Biquad ap;
};

/** Splits a signal at four edge frequencies into five phase-aligned regions:
    below | low | mid | high | above. The five outputs sum to an all-pass (flat). */
class FiveWaySplit
{
public:
    enum { Below = 0, Low, Mid, High, Above, NumRegions };

    void setFrequencies (const float (&edges)[4], double sr) noexcept
    {
        for (int i = 0; i < 4; ++i)
            xo[i].setFrequency (edges[i], sr);
        // region r needs the all-passes of every crossover above it
        int k = 0;
        for (int r = 0; r < 3; ++r)
            for (int c = r + 1; c < 4; ++c)
                ap[k++].setFrequency (edges[c], sr);
    }

    void reset() noexcept
    {
        for (auto& x : xo) x.reset();
        for (auto& a : ap) a.reset();
    }

    void process (float x, float (&out)[NumRegions]) noexcept
    {
        float rest = x, low, high;
        xo[0].split (rest, low, high); out[Below] = low; rest = high;
        xo[1].split (rest, low, high); out[Low]   = low; rest = high;
        xo[2].split (rest, low, high); out[Mid]   = low; rest = high;
        xo[3].split (rest, low, high); out[High]  = low; out[Above] = high;

        // below: AP(f1) AP(f2) AP(f3); low: AP(f2) AP(f3); mid: AP(f3)
        out[Below] = ap[2].process (ap[1].process (ap[0].process (out[Below])));
        out[Low]   = ap[4].process (ap[3].process (out[Low]));
        out[Mid]   = ap[5].process (out[Mid]);
    }

private:
    LR4Crossover xo[4];
    LR4Allpass ap[6];
};
} // namespace acid
