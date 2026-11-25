#include "Lfo.h"

namespace acid
{
namespace
{
    float baseShape (int shape, double ph) noexcept
    {
        const float p = (float) ph;
        switch (shape)
        {
            case Lfo::Sine:     return std::sin (kTwoPi * p);
            case Lfo::Triangle: return p < 0.5f ? 4.0f * p - 1.0f : 3.0f - 4.0f * p;
            case Lfo::SawUp:    return 2.0f * p - 1.0f;
            case Lfo::SawDown:  return 1.0f - 2.0f * p;
            case Lfo::Square:   return p < 0.5f ? 1.0f : -1.0f;
            default:            return 0.0f;
        }
    }

    // Fixed pseudo-random steps used to draw the random shapes.
    constexpr float kDrawRandom[9] = { 0.2f, -0.7f, 0.9f, -0.1f, 0.5f, -0.9f, 0.3f, 0.75f, 0.2f };
}

float Lfo::shapeValue (int shape, double ph) noexcept
{
    ph -= std::floor (ph);
    if (shape == SampleHold)
        return kDrawRandom[(int) (ph * 8.0) % 8];
    if (shape == SmoothRandom)
    {
        const double x = ph * 8.0;
        const int i = (int) x % 8;
        const float t = (float) (x - std::floor (x));
        const float s = 0.5f - 0.5f * std::cos (kPi * t);
        return lerp (kDrawRandom[i], kDrawRandom[i + 1], s);
    }
    return baseShape (shape, ph);
}

float Lfo::valueAt (double ph) noexcept
{
    float v;
    if (shape == SampleHold)
        v = nextRandom;
    else if (shape == SmoothRandom)
        v = lerp (prevRandom, nextRandom, 0.5f - 0.5f * std::cos (kPi * (float) ph));
    else
        v = baseShape (shape, ph);
    return mirror ? 0.5f * (v + 1.0f) : v;
}

void Lfo::process (float* out, int numSamples, double cyclesPerSecond,
                   double cyclesPerBeat, bool positionValid, double ppqStart, double ppqPerSample) noexcept
{
    const double inc = std::max (0.0, cyclesPerSecond) / sr;

    for (int i = 0; i < numSamples; ++i)
    {
        double ph;
        if (retrig == RetrigBar && positionValid)
        {
            const double cycles = (ppqStart + ppqPerSample * i) * cyclesPerBeat;
            const double newPhase = cycles - std::floor (cycles);
            if (newPhase < phase) newRandom();
            phase = newPhase;
        }
        else
        {
            phase += inc;
            if (phase >= 1.0)
            {
                phase -= std::floor (phase);
                newRandom();
            }
        }
        ph = phase + phaseOffset;
        ph -= std::floor (ph);
        out[i] = valueAt (ph);
    }
}
} // namespace acid
