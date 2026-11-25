#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "dsp/Crossover.h"
#include "dsp/MultibandTube.h"
#include <vector>

using namespace acid;

TEST_CASE ("The five crossover regions sum flat", "[tube][crossover]")
{
    const double sr = 48000.0;
    const float edges[4] = { 62.0f, 748.0f, 2100.0f, 11000.0f };

    for (float hz : { 30.0f, 62.0f, 200.0f, 748.0f, 1200.0f, 2100.0f, 5000.0f, 11000.0f, 16000.0f })
    {
        FiveWaySplit split;
        split.setFrequencies (edges, sr);
        double inPow = 0.0, outPow = 0.0;
        const int n = 48000;
        for (int i = 0; i < n; ++i)
        {
            const float x = std::sin (kTwoPi * hz * (float) i / (float) sr);
            float r[FiveWaySplit::NumRegions];
            split.process (x, r);
            float sum = 0.0f;
            for (float v : r) sum += v;
            if (i > n / 2) { inPow += x * x; outPow += sum * sum; }
        }
        const double db = 10.0 * std::log10 (outPow / inPow);
        INFO ("frequency " << hz << " Hz: " << db << " dB");
        REQUIRE_THAT (db, Catch::Matchers::WithinAbs (0.0, 0.05));
    }
}

TEST_CASE ("Tube dry path and bypass are delayed by the reported latency", "[tube]")
{
    const double sr = 48000.0;
    MultibandTube tube;
    TubeSettings s;
    s.mix = 0.0f;                // dry only
    tube.setSettings (s);
    tube.prepare (sr);

    std::vector<float> l (256, 0.0f), r (256, 0.0f);
    l[5] = r[5] = 1.0f;
    tube.process (l.data(), r.data(), 256);
    REQUIRE_THAT (l[(size_t) (5 + MultibandTube::getLatencySamples())], Catch::Matchers::WithinAbs (1.0, 1.0e-4));
}

TEST_CASE ("Tube output is finite with everything at maximum", "[tube]")
{
    MultibandTube tube;
    TubeSettings s;
    for (auto& d : s.drive) d = 1.0f;
    for (auto& g : s.bandGainDb) g = 15.0f;
    s.resonator = 1.0f; s.character = 1.0f; s.inputDb = 24.0f;
    tube.setSettings (s);
    tube.prepare (44100.0);
    FastRandom rng (3);
    std::vector<float> l (1024), r (1024);
    for (int b = 0; b < 50; ++b)
    {
        for (size_t i = 0; i < l.size(); ++i) { l[i] = rng.nextBipolar(); r[i] = rng.nextBipolar(); }
        tube.process (l.data(), r.data(), 1024);
        for (float v : l) REQUIRE (std::isfinite (v));
    }
}

TEST_CASE ("Tube curve is odd-symmetric without character", "[tube]")
{
    for (float x : { 0.1f, 0.5f, 0.9f })
        REQUIRE_THAT (MultibandTube::tubeCurve (x, 0.6f, 0.0f), Catch::Matchers::WithinAbs (-MultibandTube::tubeCurve (-x, 0.6f, 0.0f), 1.0e-5));
    REQUIRE_THAT (MultibandTube::tubeCurve (0.0f, 0.6f, 0.7f), Catch::Matchers::WithinAbs (0.0, 1.0e-6));
}

TEST_CASE ("Tube wet path is aligned with the dry path", "[tube]")
{
    // At zero drive the tube is nearly linear, so the wet output should match the
    // crossover's own (all-pass) sum delayed by exactly the reported latency.
    const double sr = 48000.0;
    MultibandTube tube;
    TubeSettings s;
    for (auto& d : s.drive) d = 0.0f;
    tube.setSettings (s);
    tube.prepare (sr);

    FiveWaySplit reference;
    reference.setFrequencies (s.edges, sr);

    FastRandom rng (11);
    const int n = 4096, lat = MultibandTube::getLatencySamples();
    std::vector<float> l (n), r (n), ref (n);
    for (int i = 0; i < n; ++i)
    {
        l[(size_t) i] = r[(size_t) i] = 0.02f * rng.nextBipolar();
        float regions[FiveWaySplit::NumRegions];
        reference.process (l[(size_t) i], regions);
        ref[(size_t) i] = 0.0f;
        for (float v : regions) ref[(size_t) i] += v;
    }
    tube.process (l.data(), r.data(), n);

    auto correlation = [&] (int shift)
    {
        double num = 0.0, a = 0.0, b = 0.0;
        for (int i = 1000; i < n - 64; ++i)
        {
            num += (double) l[(size_t) (i + shift)] * ref[(size_t) i];
            a += (double) l[(size_t) (i + shift)] * l[(size_t) (i + shift)];
            b += (double) ref[(size_t) i] * ref[(size_t) i];
        }
        return num / std::sqrt (a * b);
    };
    REQUIRE (correlation (lat) > 0.98);
    REQUIRE (correlation (lat) > correlation (lat - 1));
    REQUIRE (correlation (lat) > correlation (lat + 1));
}
