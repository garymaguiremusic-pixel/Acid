#include "PitchDetector.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace as::dsp
{
    namespace
    {
        constexpr double kMinHz = 40.0, kMaxHz = 2000.0;
        constexpr float kThreshold = 0.15f;     // YIN absolute threshold
        constexpr float kMaxAperiodicity = 0.3f;

        /** YIN on one window: returns the period in samples (0 = none) and the aperiodicity. */
        std::pair<float, float> yin (const float* x, int window, int tauMin, int tauMax, std::vector<float>& d)
        {
            d.assign ((size_t) tauMax + 2, 0.0f);

            for (int tau = 1; tau <= tauMax + 1; ++tau)
            {
                double sum = 0.0;
                for (int j = 0; j < window; ++j)
                {
                    const double diff = (double) x[j] - (double) x[j + tau];
                    sum += diff * diff;
                }
                d[(size_t) tau] = (float) sum;
            }

            // cumulative mean normalised difference
            d[0] = 1.0f;
            double running = 0.0;
            for (int tau = 1; tau <= tauMax + 1; ++tau)
            {
                running += d[(size_t) tau];
                d[(size_t) tau] = running > 0.0 ? (float) (d[(size_t) tau] * tau / running) : 1.0f;
            }

            int best = -1;
            for (int tau = tauMin; tau <= tauMax; ++tau)
            {
                if (d[(size_t) tau] < kThreshold)
                {
                    while (tau + 1 <= tauMax && d[(size_t) tau + 1] < d[(size_t) tau])
                        ++tau;
                    best = tau;
                    break;
                }
            }

            if (best < 0)
            {
                // nothing under the threshold: use the global minimum, which is reported as weakly periodic
                best = tauMin;
                for (int tau = tauMin; tau <= tauMax; ++tau)
                    if (d[(size_t) tau] < d[(size_t) best])
                        best = tau;
            }

            // parabolic interpolation around the minimum
            float period = (float) best;
            if (best > tauMin && best < tauMax)
            {
                const float a = d[(size_t) best - 1], b = d[(size_t) best], c = d[(size_t) best + 1];
                const float denom = a - 2.0f * b + c;
                if (std::abs (denom) > 1.0e-9f)
                    period += 0.5f * (a - c) / denom;
            }

            return { period, d[(size_t) best] };
        }
    }

    PitchResult detectFramePitch (const float* x, int window, double rate, std::vector<float>& scratch)
    {
        PitchResult r;
        const int tauMin = std::max (2, (int) (rate / kMaxHz));
        const int tauMax = (int) (rate / kMinHz);
        const auto [period, aperiodicity] = yin (x, window, tauMin, tauMax, scratch);
        if (period > 0.0f)
        {
            r.frequency = (float) (rate / period);
            r.midiNote = 69.0f + 12.0f * std::log2 (r.frequency / 440.0f);
        }
        r.confidence = std::clamp (1.0f - aperiodicity, 0.0f, 1.0f);
        r.pitched = period > 0.0f && aperiodicity < kMaxAperiodicity;
        return r;
    }

    PitchResult detectPitch (const float* mono, int numFrames, double sampleRate)
    {
        PitchResult result;
        if (mono == nullptr || numFrames < 64 || sampleRate <= 0.0)
            return result;

        // Decimate to about 16 kHz (box filter) to keep the search cheap.
        const int factor = std::max (1, (int) (sampleRate / 16000.0));
        const double rate = sampleRate / factor;
        const int frames = std::min (numFrames / factor, (int) (rate * 4.0));   // the first 4 s are plenty
        std::vector<float> x ((size_t) frames);
        float peak = 0.0f;
        int peakIndex = 0;
        for (int i = 0; i < frames; ++i)
        {
            float sum = 0.0f;
            for (int k = 0; k < factor; ++k)
                sum += mono[i * factor + k];
            x[(size_t) i] = sum / (float) factor;
            if (std::abs (x[(size_t) i]) > peak)
            {
                peak = std::abs (x[(size_t) i]);
                peakIndex = i;
            }
        }

        if (peak < 1.0e-4f)
            return result;     // silence

        const int tauMin = std::max (2, (int) (rate / kMaxHz));
        const int tauMax = (int) (rate / kMinHz);
        const int window = std::max (1024, tauMax * 3);
        const int hop = window / 2;
        const int start = std::min (peakIndex + (int) (0.03 * rate), std::max (0, frames - window - tauMax - 2));

        std::vector<float> d, periods, aperiodicities;
        for (int pos = start; pos + window + tauMax + 2 <= frames && periods.size() < 8; pos += hop)
        {
            // skip windows that have decayed into the noise floor
            double energy = 0.0;
            for (int j = 0; j < window; ++j)
                energy += (double) x[(size_t) (pos + j)] * x[(size_t) (pos + j)];
            if (std::sqrt (energy / window) < peak * 0.02f)
                break;

            const auto [period, aperiodicity] = yin (x.data() + pos, window, tauMin, tauMax, d);
            periods.push_back (period);
            aperiodicities.push_back (aperiodicity);
        }

        if (periods.empty())
            return result;

        // median period among the periodic windows
        std::vector<float> good;
        float apSum = 0.0f;
        for (size_t i = 0; i < periods.size(); ++i)
            if (aperiodicities[i] < kMaxAperiodicity)
            {
                good.push_back (periods[i]);
                apSum += aperiodicities[i];
            }

        // most windows must be periodic (a single window is accepted for very short samples)
        if (good.empty() || (periods.size() > 1 && good.size() * 2 < periods.size() + 1))
            return result;

        std::nth_element (good.begin(), good.begin() + (long) good.size() / 2, good.end());
        const float medianPeriod = good[good.size() / 2];

        // and they must agree (within half a semitone)
        int agreeing = 0;
        for (auto p : good)
            if (std::abs (12.0f * std::log2 (p / medianPeriod)) < 0.5f)
                ++agreeing;
        if (agreeing * 3 < (int) good.size() * 2)
            return result;

        result.frequency = (float) (rate / medianPeriod);
        result.midiNote = 69.0f + 12.0f * std::log2 (result.frequency / 440.0f);
        result.confidence = std::clamp (1.0f - apSum / (float) good.size(), 0.0f, 1.0f);
        result.pitched = result.frequency >= kMinHz && result.frequency <= kMaxHz;
        return result;
    }

    PitchResult detectPitch (const SampleData& s)
    {
        if (s.isEmpty())
            return {};

        std::vector<float> mono ((size_t) s.numFrames);
        const int channels = s.getNumChannels();
        for (int ch = 0; ch < channels; ++ch)
        {
            const float* src = s.getChannel (ch);
            for (int i = 0; i < s.numFrames; ++i)
                mono[(size_t) i] += src[i] / (float) channels;
        }
        return detectPitch (mono.data(), s.numFrames, s.sampleRate);
    }

    KeyLock keyLockedRoot (float detectedMidiNote, int key) noexcept
    {
        const int target = 60 + std::clamp (key, 0, 11);
        const int nearest = (int) std::lround (detectedMidiNote);
        const float cents = (detectedMidiNote - (float) nearest) * 100.0f;

        int d = ((nearest - target) % 12 + 12) % 12;   // pitch-class distance, 0..11
        if (d > 6)
            d -= 12;                                    // -5..6: the smallest shift

        KeyLock k;
        k.rootNote = std::clamp (target + d, 0, 127);
        k.fineCents = std::clamp ((int) std::lround (-cents), -100, 100);
        return k;
    }
}
