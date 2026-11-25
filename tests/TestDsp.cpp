#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "dsp/Ladder.h"
#include "dsp/Voice.h"
#include "dsp/Envelopes.h"
#include "dsp/Oscillator.h"
#include "dsp/Echo.h"
#include "dsp/Lfo.h"

using namespace acid;

TEST_CASE ("Ladder stays stable at maximum resonance", "[dsp][filter]")
{
    for (auto model : { Ladder::Model::Diode, Ladder::Model::Transistor })
    {
        for (double sr : { 88200.0, 96000.0, 192000.0 })
        {
            Ladder f;
            f.setModel (model);
            f.prepare (sr);
            FastRandom rng (7);
            float peak = 0.0f;
            for (int i = 0; i < (int) sr * 2; ++i)
            {
                // cutoff sweeping over the whole range, loud noise bursts
                const float cutoff = 20.0f * std::pow (1000.0f, 0.5f + 0.5f * std::sin (i * 0.0003f));
                const float x = (i % 20000) < 2000 ? rng.nextBipolar() * 4.0f : 0.0f;
                const float y = f.process (x, cutoff, 1.0f);
                REQUIRE (std::isfinite (y));
                peak = std::max (peak, std::abs (y));
            }
            REQUIRE (peak < 20.0f);
        }
    }
}

TEST_CASE ("Diode ladder does not self-oscillate, transistor ladder does", "[dsp][filter]")
{
    auto ringLevel = [] (Ladder::Model model)
    {
        Ladder f;
        f.setModel (model);
        f.prepare (88200.0);
        float level = 0.0f;
        for (int i = 0; i < 88200; ++i)
        {
            const float y = f.process (i == 0 ? 1.0f : 0.0f, 1000.0f, 1.0f);
            if (i > 80000) level = std::max (level, std::abs (y));
        }
        return level;
    };
    REQUIRE (ringLevel (Ladder::Model::Diode) < 1.0e-3f);
    REQUIRE (ringLevel (Ladder::Model::Transistor) > 0.05f);
}

TEST_CASE ("Accent sweep builds up over consecutive accented notes", "[dsp][accent]")
{
    const double sr = 48000.0;
    const int stepSamples = (int) (sr * 0.125);   // 16ths at 120 BPM

    auto peaks = [&] (float sweep)
    {
        FilterEnvelope env;
        env.prepare (sr);
        AccentCircuit circuit;
        circuit.prepare (sr);
        circuit.setSweep (sweep);
        std::vector<float> result;
        for (int note = 0; note < 6; ++note)
        {
            env.trigger (3.0f, 200.0f);
            float peak = 0.0f;
            for (int i = 0; i < stepSamples; ++i)
                peak = std::max (peak, circuit.process (env.process() * 1.0f));
            result.push_back (peak);
        }
        return result;
    };

    const auto noSweep = peaks (0.0f);
    for (size_t i = 1; i < noSweep.size(); ++i)
        REQUIRE_THAT (noSweep[i], Catch::Matchers::WithinAbs (noSweep[0], 1.0e-3));   // no memory

    const auto sweep = peaks (0.8f);
    for (size_t i = 1; i < sweep.size(); ++i)
        REQUIRE (sweep[i] > sweep[i - 1]);       // each accent pushes higher
    REQUIRE (sweep.back() > sweep.front() * 1.3f);
}

TEST_CASE ("Slide glides to the new pitch in about the slide time", "[dsp][slide]")
{
    const double sr = 48000.0;
    Voice v;
    v.prepare (sr);
    VoiceSettings s;
    s.slideMs = 60.0f;
    v.setSettings (s);

    std::vector<float> l (4800), r (4800);
    v.noteOn (48, false, false);
    v.render (l.data(), r.data(), 480, {}, 0);
    REQUIRE_THAT (v.getCurrentPitch(), Catch::Matchers::WithinAbs (48.0, 1.0e-3));

    v.noteOn (60, false, true);                 // slide up an octave
    v.render (l.data(), r.data(), (int) (sr * 0.020), {}, 0);
    const float after20 = v.getCurrentPitch();
    REQUIRE (after20 > 49.0f);
    REQUIRE (after20 < 59.0f);                  // still gliding

    v.render (l.data(), r.data(), (int) (sr * 0.040), {}, 0);   // 60 ms in total
    REQUIRE (v.getCurrentPitch() > 60.0f - 12.0f * 0.06f);      // ~95 % there

    // a non-legato note jumps
    v.noteOff();
    v.noteOn (40, false, false);
    REQUIRE_THAT (v.getCurrentPitch(), Catch::Matchers::WithinAbs (40.0, 1.0e-3));
}

TEST_CASE ("Voice output is finite at every sample rate and setting extreme", "[dsp][voice]")
{
    for (double sr : { 44100.0, 48000.0, 96000.0 })
    {
        Voice v;
        v.prepare (sr);
        VoiceSettings s;
        s.resonance = 1.0f; s.envMod = 1.0f; s.accent = 1.0f; s.accSweep = 1.0f; s.drive = 1.0f;
        s.unisonVoices = 8; s.unisonSpread = 1.0f; s.oscBLevel = 1.0f; s.waveMorph = 1.0f; s.retro = true;
        v.setSettings (s);
        std::vector<float> l (2048), r (2048);
        float peak = 0.0f;
        for (int block = 0; block < 100; ++block)
        {
            if (block % 4 == 0) v.noteOn (24 + block % 48, block % 3 == 0, block % 8 == 4);
            if (block % 4 == 2) v.noteOff();
            const int n = 1 + (block * 37) % 2048;   // odd block sizes
            v.render (l.data(), r.data(), n, {}, 0);
            for (int i = 0; i < n; ++i)
            {
                REQUIRE (std::isfinite (l[(size_t) i]));
                REQUIRE (std::isfinite (r[(size_t) i]));
                peak = std::max (peak, std::abs (l[(size_t) i]));
            }
        }
        REQUIRE (peak > 0.01f);
        REQUIRE (peak < 8.0f);
    }
}

TEST_CASE ("Half-band oversampler has unity gain and the stated latency", "[dsp][oversampling]")
{
    Halfband2x up (31), down (31);
    std::vector<float> out;
    for (int i = 0; i < 200; ++i)
    {
        float a, b;
        up.upsample (i == 10 ? 1.0f : 0.0f, a, b);
        out.push_back (down.downsample (a, b));
    }
    const auto peakIndex = std::max_element (out.begin(), out.end(), [] (float x, float y) { return std::abs (x) < std::abs (y); }) - out.begin();
    REQUIRE (peakIndex == 10 + up.getLatencyLowRate());

    // DC passes at unity
    Halfband2x u2 (31), d2 (31);
    float y = 0.0f;
    for (int i = 0; i < 200; ++i) { float a, b; u2.upsample (1.0f, a, b); y = d2.downsample (a, b); }
    REQUIRE_THAT (y, Catch::Matchers::WithinAbs (1.0, 1.0e-3));
}

TEST_CASE ("Echo rings out after being switched off", "[dsp][echo]")
{
    const double sr = 48000.0;
    Echo e;
    EchoSettings s;
    s.timeMs = 100.0f; s.feedback = 0.6f; s.mix = 0.5f;
    e.prepare (sr);
    e.setSettings (s);

    std::vector<float> l (4800, 0.0f), r (4800, 0.0f);
    l[0] = r[0] = 1.0f;
    e.process (l.data(), r.data(), 4800);       // input goes in

    s.on = false;
    e.setSettings (s);
    std::fill (l.begin(), l.end(), 0.0f);
    std::fill (r.begin(), r.end(), 0.0f);
    e.process (l.data(), r.data(), 4800);
    float tail = 0.0f;
    for (float v : l) tail = std::max (tail, std::abs (v));
    REQUIRE (tail > 0.01f);                     // repeats still sound
}

TEST_CASE ("LFO bar retrigger follows the song position", "[dsp][lfo]")
{
    Lfo lfo;
    lfo.prepare (48000.0);
    lfo.setShape (Lfo::SawUp);
    lfo.setRetrig (Lfo::RetrigBar);
    float out[4];
    const double ppqPerSample = 2.0 / 48000.0;  // 120 BPM
    lfo.process (out, 4, 0.0, 0.25, true, 2.0, ppqPerSample);   // one cycle per bar, at beat 2
    REQUIRE_THAT (out[0], Catch::Matchers::WithinAbs (0.0, 1.0e-3));   // halfway through the bar = 0 on a rising saw
}

TEST_CASE ("Vel Link: lower velocity closes the cutoff, 127 leaves it, a tie slides it", "[dsp][velocity]")
{
    const double sr = 48000.0;
    auto cutoffAfter = [&] (bool link, float amount, float velocity)
    {
        Voice v;
        v.prepare (sr);
        VoiceSettings s;
        s.envMod = 0.0f;
        s.cutoffLowHz = 20000.0f;     // no envelope sweep: the cutoff sits on the knob
        s.velLink = link;
        s.velAmount = amount;
        v.setSettings (s);
        std::vector<float> l (480), r (480);
        v.noteOn (48, false, false, 1.0f, velocity);
        v.render (l.data(), r.data(), 480, {}, 0);
        return v.getLastCutoffHz();
    };

    const float base = cutoffAfter (false, 1.0f, 0.2f);
    REQUIRE (std::abs (cutoffAfter (true, 1.0f, 1.0f) - base) < base * 0.01f);         // 127: no change
    const float closed = cutoffAfter (true, 1.0f, 0.0f);
    REQUIRE (std::abs (std::log2 (base / closed) - 4.0f) < 0.05f);                     // 100 %: 4 octaves
    const float half = cutoffAfter (true, 0.5f, 0.0f);
    REQUIRE (std::abs (std::log2 (base / half) - 2.0f) < 0.05f);                        // 50 %: 2 octaves

    // a tied (legato) step slides the cutoff to the new velocity instead of jumping
    Voice v;
    v.prepare (sr);
    VoiceSettings s;
    s.envMod = 0.0f; s.cutoffLowHz = 20000.0f; s.velLink = true; s.velAmount = 1.0f; s.slideMs = 60.0f;
    v.setSettings (s);
    std::vector<float> l (4800), r (4800);
    v.noteOn (48, false, false, 1.0f, 1.0f);
    v.render (l.data(), r.data(), 480, {}, 0);
    const float open = v.getLastCutoffHz();
    v.noteOn (50, false, true, 1.0f, 0.0f);
    v.render (l.data(), r.data(), 96, {}, 0);          // 2 ms in: still near the old cutoff
    REQUIRE (v.getLastCutoffHz() > open * 0.5f);
    v.render (l.data(), r.data(), 4800, {}, 0);        // 100 ms later: arrived
    REQUIRE (v.getLastCutoffHz() < open / 12.0f);
}
