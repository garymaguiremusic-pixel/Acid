#include "Echo.h"

namespace acid
{
void Echo::prepare (double sampleRate)
{
    sr = sampleRate;
    size = (int) (kMaxDelaySeconds * sr) + 4;
    bufL.assign ((size_t) size, 0.0f);
    bufR.assign ((size_t) size, 0.0f);
    xfadeInc = 1.0f / (0.05f * (float) sr);     // 50 ms crossfade
    inputGate.prepare (sr, 10.0f);
    mixSmooth.prepare (sr, 30.0f);
    fbSmooth.prepare (sr, 30.0f);
    widthSmooth.prepare (sr, 30.0f);
    setSettings (set);
    headTime[0] = headTime[1] = clampf (set.timeMs * 0.001f * (float) sr, 1.0f, (float) size - 4.0f);
    inputGate.snap (set.on ? 1.0f : 0.0f);
    mixSmooth.snap (set.mix);
    fbSmooth.snap (set.feedback);
    widthSmooth.snap (set.width);
    reset();
}

void Echo::reset()
{
    std::fill (bufL.begin(), bufL.end(), 0.0f);
    std::fill (bufR.begin(), bufR.end(), 0.0f);
    writePos = 0;
    for (auto* f : { &lowCutL, &lowCutR, &highCutL, &highCutR })
        f->reset();
    xfade = 1.0f;
    pendingTime = -1.0f;
}

void Echo::setSettings (const EchoSettings& s) noexcept
{
    set = s;
    lowCutL.setCutoff (s.lowCutHz, sr);   lowCutR.setCutoff (s.lowCutHz, sr);
    highCutL.setCutoff (s.highCutHz, sr); highCutR.setCutoff (s.highCutHz, sr);
    inputGate.setTarget (s.on ? 1.0f : 0.0f);
    mixSmooth.setTarget (clampf (s.mix, 0.0f, 1.0f));
    fbSmooth.setTarget (clampf (s.feedback, 0.0f, 0.98f));
    widthSmooth.setTarget (clampf (s.width, 0.0f, 1.0f));

    const float t = clampf (s.timeMs * 0.001f * (float) sr, 1.0f, (float) size - 4.0f);
    if (std::abs (t - headTime[activeHead]) > 0.5f)
        pendingTime = t;
}

float Echo::read (const std::vector<float>& buf, float delay) const noexcept
{
    float pos = (float) writePos - delay;
    while (pos < 0.0f) pos += (float) size;
    const int i0 = (int) pos;
    const int i1 = (i0 + 1) % size;
    const float frac = pos - (float) i0;
    return buf[(size_t) i0] + (buf[(size_t) i1] - buf[(size_t) i0]) * frac;
}

void Echo::process (float* left, float* right, int numSamples) noexcept
{
    for (int i = 0; i < numSamples; ++i)
    {
        // start a crossfade to the new time once the previous one has finished
        if (pendingTime > 0.0f && xfade >= 1.0f)
        {
            activeHead ^= 1;
            headTime[activeHead] = pendingTime;
            pendingTime = -1.0f;
            xfade = 0.0f;
        }

        float dl, dr;
        {
            const int a = activeHead, b = activeHead ^ 1;
            dl = read (bufL, headTime[a]);
            dr = read (bufR, headTime[a]);
            if (xfade < 1.0f)
            {
                const float g = 0.5f - 0.5f * std::cos (kPi * xfade);
                dl = read (bufL, headTime[b]) * (1.0f - g) + dl * g;
                dr = read (bufR, headTime[b]) * (1.0f - g) + dr * g;
                xfade = std::min (1.0f, xfade + xfadeInc);
            }
        }

        const float fb = fbSmooth.next(), w = widthSmooth.next(), mix = mixSmooth.next();
        const float gate = inputGate.next();
        const float in = gate * 0.5f * (left[i] + right[i]);

        // loop filters, then soft clip
        float fl = highCutL.lowpass (lowCutL.highpass (dl));
        float fr = highCutR.lowpass (lowCutR.highpass (dr));

        const float toL = in + fb * ((1.0f - w) * fl + w * fr);
        const float toR = (1.0f - w) * in + fb * ((1.0f - w) * fr + w * fl);
        bufL[(size_t) writePos] = 1.2f * fastTanh (toL / 1.2f);
        bufR[(size_t) writePos] = 1.2f * fastTanh (toR / 1.2f);
        writePos = (writePos + 1) % size;

        // equal-power dry / wet
        // (while off, the dry signal returns to full level and the tail rings out)
        const float dryG = 1.0f + (std::cos (mix * kPi * 0.5f) - 1.0f) * gate, wetG = std::sin (mix * kPi * 0.5f);
        left[i]  = left[i] * dryG + dl * wetG;
        right[i] = right[i] * dryG + dr * wetG;
    }
}
} // namespace acid
