#include "PatternTools.h"
#include "../dsp/DspCommon.h"

namespace acid::PatternTools
{
const std::vector<Scale>& scales()
{
    static const std::vector<Scale> list =
    {
        { "Chromatic",      { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 } },
        { "Minor",          { 0, 2, 3, 5, 7, 8, 10 } },
        { "Major",          { 0, 2, 4, 5, 7, 9, 11 } },
        { "Phrygian",       { 0, 1, 3, 5, 7, 8, 10 } },
        { "Dorian",         { 0, 2, 3, 5, 7, 9, 10 } },
        { "Minor Pentatonic", { 0, 3, 5, 7, 10 } },
        { "Harmonic Minor", { 0, 2, 3, 5, 7, 8, 11 } },
        { "Blues",          { 0, 3, 5, 6, 7, 10 } },
    };
    return list;
}

const char* rootName (int root)
{
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return names[((root % 12) + 12) % 12];
}

namespace
{
    int pickNote (FastRandom& rng, int root, int scaleIndex)
    {
        const auto& sc = scales()[(size_t) std::clamp (scaleIndex, 0, (int) scales().size() - 1)];
        int interval = 0;
        if (rng.nextFloat() >= 0.3f)   // root 30 % of the time
            interval = sc.intervals[rng.nextInt() % sc.intervals.size()];
        int note = (root + interval) % 12;
        if (note == 0 && rng.nextFloat() < 0.25f)
            note = 12;                  // the high C now and then
        return note;
    }

    int8_t pickOctave (FastRandom& rng)
    {
        const float r = rng.nextFloat();
        if (r < 0.68f) return 0;
        if (r < 0.86f) return 1;
        if (r < 0.97f) return -1;
        return 2;
    }
}

void randomise (Pattern& p, RandomMode mode, int root, int scaleIndex, uint32_t seed)
{
    FastRandom rng (seed * 2654435761u + 17u);
    for (int i = 0; i < kMaxSteps; ++i)
    {
        auto& st = p.steps[(size_t) i];
        const bool all = mode == RandomMode::All;

        if (all || mode == RandomMode::Notes)
            st.note = (uint8_t) pickNote (rng, root, scaleIndex);
        if (all || mode == RandomMode::Octaves)
            st.octave = pickOctave (rng);
        if (all || mode == RandomMode::Accents)
            st.accent = rng.nextFloat() < 0.3f;
        if (all || mode == RandomMode::Slides)
            st.slide = rng.nextFloat() < 0.22f;
        if (all)
        {
            const float g = rng.nextFloat();
            st.gate = g < 0.14f ? Gate::Rest : (g < 0.19f && i > 0 ? Gate::Tie : Gate::Note);
            st.ratchet = rng.nextFloat() < 0.05f ? 1 : 0;
        }
    }
}

void mutate (Pattern& p, int root, int scaleIndex, uint32_t seed, float amount)
{
    FastRandom rng (seed * 2246822519u + 3u);
    for (int i = 0; i < p.length; ++i)
    {
        if (rng.nextFloat() >= amount)
            continue;
        auto& st = p.steps[(size_t) i];
        switch (rng.nextInt() % 5)
        {
            case 0:  st.note = (uint8_t) pickNote (rng, root, scaleIndex); break;
            case 1:  st.octave = pickOctave (rng); break;
            case 2:  st.accent = ! st.accent; break;
            case 3:  st.slide = ! st.slide; break;
            default: st.gate = st.gate == Gate::Rest ? Gate::Note : (rng.nextFloat() < 0.5f ? Gate::Rest : Gate::Note); break;
        }
    }
}

void rotate (Pattern& p, int direction)
{
    const int len = std::clamp (p.length, 1, kMaxSteps);
    if (direction > 0)
        std::rotate (p.steps.begin(), p.steps.begin() + (len - 1), p.steps.begin() + len);
    else
        std::rotate (p.steps.begin(), p.steps.begin() + 1, p.steps.begin() + len);
}

void clear (Pattern& p)
{
    const int len = p.length;
    p = Pattern {};
    p.length = len;
}
} // namespace acid::PatternTools
