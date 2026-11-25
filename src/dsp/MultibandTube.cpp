#include "MultibandTube.h"

namespace acid
{
float MultibandTube::tubeCurve (float x, float drive01, float character01) noexcept
{
    const float g = 0.3f + 24.0f * drive01 * drive01;
    const float b = 0.85f * character01;
    const float tb = std::tanh (b);
    const float norm = std::tanh (g + b) - tb;
    return (std::tanh (g * x + b) - tb) / norm;
}

void MultibandTube::Band::reset()
{
    resonator.reset();
    for (auto* h : { &up1, &up2, &down1, &down2 })
        h->reset();
    dc.reset();
}

float MultibandTube::Band::process (float x, float drive, float character, float resAmount) noexcept
{
    if (resAmount > 0.0f)
        x += resAmount * 2.5f * resonator.process (x);

    float a, b, s[4];
    up1.upsample (x, a, b);
    up2.upsample (a, s[0], s[1]);      // one continuous 2x stream: a, b, a, b ...
    up2.upsample (b, s[2], s[3]);

    for (auto& v : s)
        v = tubeCurve (v, drive, character);

    const float da = down2.downsample (s[0], s[1]);
    const float db = down2.downsample (s[2], s[3]);
    return dc.process (down1.downsample (da, db));
}

void MultibandTube::prepare (double sampleRate)
{
    sr = sampleRate;
    for (auto& ch : bands)
        for (auto& b : ch)
            b.dc.prepare (sr);
    activeFade.prepare (sr, 20.0f);
    mixSmooth.prepare (sr, 30.0f);
    inGain.prepare (sr, 30.0f);
    outGain.prepare (sr, 30.0f);
    for (auto& g : bandGain) g.prepare (sr, 30.0f);
    levelCoef = onePoleCoef (60.0f, sr);
    for (auto& e : lastEdges) e = 0.0f;
    setSettings (set);
    activeFade.snap (set.active ? 1.0f : 0.0f);
    mixSmooth.snap (set.mix);
    inGain.snap (dbToGain (set.inputDb));
    outGain.snap (dbToGain (set.outputDb));
    reset();
}

void MultibandTube::reset()
{
    for (auto& s : split) s.reset();
    for (auto& ch : bands)
        for (auto& b : ch)
            b.reset();
    for (int c = 0; c < 2; ++c) { cleanDelay[c].reset(); dryDelay[c].reset(); }
}

void MultibandTube::setSettings (const TubeSettings& s)
{
    set = s;

    // keep the edges ordered and inside the audio band
    float e[4];
    e[0] = clampf (s.edges[0], 20.0f, 18000.0f);
    for (int i = 1; i < 4; ++i)
        e[i] = clampf (std::max (s.edges[i], e[i - 1] * 1.05f), 20.0f, 0.45f * (float) sr);

    bool changed = false;
    for (int i = 0; i < 4; ++i)
        changed |= std::abs (e[i] - lastEdges[i]) > 0.01f;

    if (changed)
    {
        for (int i = 0; i < 4; ++i) lastEdges[i] = e[i];
        for (auto& sp : split)
            sp.setFrequencies (e, sr);

        const float centres[3] = { std::sqrt (e[0] * e[1]), std::sqrt (e[1] * e[2]), std::sqrt (e[2] * e[3]) };
        for (auto& ch : bands)
            for (int b = 0; b < 3; ++b)
                ch[b].resonator.setBandpass (centres[b], 1.5f + 4.0f * s.resonator, sr);
    }

    activeFade.setTarget (s.active ? 1.0f : 0.0f);
    mixSmooth.setTarget (clampf (s.mix, 0.0f, 1.0f));
    inGain.setTarget (dbToGain (s.inputDb));
    outGain.setTarget (dbToGain (s.outputDb));
    for (int b = 0; b < 3; ++b)
        bandGain[b].setTarget (dbToGain (s.bandGainDb[b]));
}

void MultibandTube::process (float* left, float* right, int numSamples) noexcept
{
    const int latency = getLatencySamples();
    const bool anySolo = set.solo[0] || set.solo[1] || set.solo[2];
    float* io[2] = { left, right };

    for (int i = 0; i < numSamples; ++i)
    {
        const float fade = activeFade.next();
        const float mix = mixSmooth.next();
        const float gi = inGain.next(), go = outGain.next();
        float bg[3];
        for (int b = 0; b < 3; ++b) bg[b] = bandGain[b].next();

        for (int c = 0; c < 2; ++c)
        {
            const float x = io[c][i];
            const float dry = dryDelay[c].process (x, latency);

            float regions[FiveWaySplit::NumRegions];
            split[c].process (x * gi, regions);

            float wet = 0.0f;
            for (int b = 0; b < 3; ++b)
            {
                // The tube curve has ~3x small-signal gain at the default drive: scale the
                // wet path to about unity there, and let more drive mean more saturation
                // rather than just more level.
                const float makeup = 0.33f / (1.0f + 1.6f * set.drive[b] * set.drive[b]);
                const float y = bands[c][b].process (regions[FiveWaySplit::Low + b], set.drive[b], set.character, set.resonator) * bg[b] * makeup;
                if (! anySolo || set.solo[b])
                    wet += y;
                if (c == 0)
                    bandLevel[b] += (std::min (1.5f, std::abs (y)) - bandLevel[b]) * levelCoef;
            }

            const float clean = cleanDelay[c].process (regions[FiveWaySplit::Below] + regions[FiveWaySplit::Above], latency);
            if (! anySolo)
                wet += clean;

            const float processed = dry + (wet * go - dry) * mix;
            io[c][i] = dry + (processed - dry) * fade;
        }
    }
}
} // namespace acid
