#pragma once

#include "DspCommon.h"

namespace acid
{
struct EchoSettings
{
    bool  on          = true;
    float timeMs      = 350.0f;
    float mix         = 0.15f;    // 0..1, equal-power
    float feedback    = 0.43f;    // 0..0.98
    float lowCutHz    = 80.0f;
    float highCutHz   = 4300.0f;
    float width       = 0.8f;     // 0 = mono echo, 1 = full ping-pong
};

/** Stereo echo. Low cut and high cut sit inside the feedback loop (every repeat gets
    thinner and darker), the loop is soft-clipped, Width cross-feeds the two lines into
    a ping-pong. Time changes crossfade between two read heads (no pitch zipper).
    Turning it off stops new input but lets the repeats ring out. */
class Echo
{
public:
    static constexpr float kMaxDelaySeconds = 4.0f;

    void prepare (double sampleRate);
    void reset();
    void setSettings (const EchoSettings&) noexcept;
    void process (float* left, float* right, int numSamples) noexcept;

private:
    float read (const std::vector<float>& buf, float delaySamples) const noexcept;

    double sr = 44100.0;
    EchoSettings set;
    std::vector<float> bufL, bufR;
    int writePos = 0, size = 1;

    float headTime[2] { 1000.0f, 1000.0f };   // delay of each head, in samples
    int activeHead = 0;
    float xfade = 1.0f, xfadeInc = 0.001f;    // 1 = fully on the active head
    float pendingTime = -1.0f;

    OnePole lowCutL, lowCutR, highCutL, highCutR;
    Smoother inputGate, mixSmooth, fbSmooth, widthSmooth;
};
} // namespace acid
