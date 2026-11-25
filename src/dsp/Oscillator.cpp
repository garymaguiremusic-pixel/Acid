#include "Oscillator.h"

namespace acid
{
namespace
{
    // PolyBLEP residual for a unit step at phase 0 (t = phase, dt = increment).
    inline float polyBlep (double t, double dt) noexcept
    {
        if (t < dt)       { t /= dt;               return (float) (t + t - t * t - 1.0); }
        if (t > 1.0 - dt) { t = (t - 1.0) / dt;    return (float) (t * t + t + t + 1.0); }
        return 0.0f;
    }

    constexpr float kSquareDrive = 9.0f;     // steepness of the shaper (rounded edges)
    constexpr float kSquareBias  = 0.06f;    // slightly asymmetric duty cycle
    constexpr float kSquareTilt  = 0.18f;    // tilted top / bottom
}

float Oscillator::squareFromSaw (float saw) noexcept
{
    // A falling saw makes the square start high, like the hardware.
    const float shaped = fastTanh (kSquareDrive * (kSquareBias - saw));
    return 0.82f * (shaped + kSquareTilt * saw) / (1.0f + kSquareTilt);
}

float Oscillator::process (float morph) noexcept
{
    float saw = (float) (2.0 * ph - 1.0) - polyBlep (ph, inc);
    ph += inc;
    if (ph >= 1.0) ph -= 1.0;

    morph = clampf (morph, 0.0f, 1.0f);
    if (morph <= 0.0f)
        return saw;

    const float sq = squareFromSaw (saw);
    return saw + (sq - saw) * morph;
}

float Oscillator::shapeAt (float phase, float morph) noexcept
{
    const float saw = 2.0f * (phase - std::floor (phase)) - 1.0f;
    morph = clampf (morph, 0.0f, 1.0f);
    return saw + (squareFromSaw (saw) - saw) * morph;
}
} // namespace acid
