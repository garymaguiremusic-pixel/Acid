#include "AudioToNotes.h"
#include "PitchDetector.h"

namespace as::import
{
    using namespace juce;

    std::vector<ImportedNote> detectNotes (const AudioBuffer<float>& audio, double sampleRate, double bpm)
    {
        std::vector<ImportedNote> notes;
        const int frames = audio.getNumSamples();
        if (frames < 256 || sampleRate <= 0.0 || audio.getNumChannels() == 0)
            return notes;

        // mono, decimated to about 16 kHz (box filter) to keep the pitch search cheap
        const int factor = jmax (1, (int) (sampleRate / 16000.0));
        const double rate = sampleRate / factor;
        const int n = frames / factor;
        std::vector<float> x ((size_t) n);
        for (int i = 0; i < n; ++i)
        {
            float sum = 0.0f;
            for (int ch = 0; ch < audio.getNumChannels(); ++ch)
                for (int k = 0; k < factor; ++k)
                    sum += audio.getSample (ch, i * factor + k);
            x[(size_t) i] = sum / (float) (factor * audio.getNumChannels());
        }

        // frame analysis: 40 ms windows every 10 ms
        const int window = (int) (rate * 0.040);
        const int hop = jmax (1, (int) (rate * 0.010));
        const int lag = (int) (rate / 40.0) + 2;
        struct Frame { double time; float rms; float midi; bool pitched; };
        std::vector<Frame> f;
        std::vector<float> scratch;
        float peak = 0.0f;
        for (int pos = 0; pos + window + lag <= n; pos += hop)
        {
            double e = 0.0;
            for (int j = 0; j < window; ++j)
                e += (double) x[(size_t) (pos + j)] * x[(size_t) (pos + j)];
            const float rms = (float) std::sqrt (e / window);
            const auto p = dsp::detectFramePitch (x.data() + pos, window, rate, scratch);
            f.push_back ({ (pos + window * 0.25) / rate, rms, p.midiNote, p.pitched });
            peak = jmax (peak, rms);
        }
        if (f.empty() || peak < 1.0e-4f)
            return notes;

        const float floor = peak * 0.06f;      // about -24 dB under the loudest part
        auto voiced = [&] (const Frame& fr) { return fr.pitched && fr.rms > floor; };

        // walk the frames, opening and closing notes
        int startFrame = -1, quiet = 0;
        std::vector<float> pitches;
        float notePeak = 0.0f;

        auto close = [&] (int endFrame)
        {
            if (startFrame < 0 || pitches.empty())
            {
                startFrame = -1;
                return;
            }
            std::vector<float> sorted (pitches);
            std::nth_element (sorted.begin(), sorted.begin() + (long) sorted.size() / 2, sorted.end());
            const int note = jlimit (0, 127, roundToInt (sorted[sorted.size() / 2]));
            const double t0 = f[(size_t) startFrame].time;
            const double t1 = f[(size_t) jmax (startFrame, endFrame)].time + hop / rate;
            if (t1 - t0 >= 0.035)
            {
                const float db = Decibels::gainToDecibels (notePeak / peak, -60.0f);
                const int velocity = jlimit (1, 127, roundToInt (jmap (jlimit (-30.0f, 0.0f, db), -30.0f, 0.0f, 40.0f, 127.0f)));
                notes.push_back ({ t0 * bpm / 60.0, (t1 - t0) * bpm / 60.0, note, velocity });
            }
            startFrame = -1;
            pitches.clear();
            notePeak = 0.0f;
        };

        for (int i = 0; i < (int) f.size(); ++i)
        {
            const auto& fr = f[(size_t) i];
            if (! voiced (fr))
            {
                if (startFrame >= 0 && ++quiet >= 2)
                    close (i - quiet);
                continue;
            }
            quiet = 0;

            if (startFrame >= 0)
            {
                // a new note: the pitch has moved (for two frames running), or the level jumped
                const float current = pitches[pitches.size() / 2];
                const bool moved = std::abs (fr.midi - current) > 0.7f
                                && i + 1 < (int) f.size() && voiced (f[(size_t) i + 1])
                                && std::abs (f[(size_t) i + 1].midi - fr.midi) < 0.7f;
                const bool attack = i >= 3 && fr.rms > 1.8f * f[(size_t) i - 3].rms && fr.rms > peak * 0.15f;
                if (moved || attack)
                    close (i - 1);
            }

            if (startFrame < 0)
                startFrame = i;
            pitches.push_back (fr.midi);
            notePeak = jmax (notePeak, fr.rms);
        }
        close ((int) f.size() - 1);
        return notes;
    }

    std::vector<ImportedNote> detectNotesInFile (const File& file, double bpm, double maxSeconds)
    {
        AudioFormatManager formats;
        formats.registerBasicFormats();
        std::unique_ptr<AudioFormatReader> reader (formats.createReaderFor (file));
        if (reader == nullptr || reader->sampleRate <= 0.0)
            return {};

        const auto length = (int) jmin ((int64) (reader->sampleRate * maxSeconds), reader->lengthInSamples);
        AudioBuffer<float> audio ((int) jlimit (1u, 2u, reader->numChannels), jmax (1, length));
        reader->read (&audio, 0, length, 0, true, audio.getNumChannels() > 1);
        return detectNotes (audio, reader->sampleRate, bpm);
    }
}
