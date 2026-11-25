#include "PatternImport.h"
#include "../Scales.h"

namespace as::import
{
    using namespace juce;

    std::vector<ImportedNote> readMidiNotes (const MidiFile& source)
    {
        std::vector<ImportedNote> notes;
        const int ppq = source.getTimeFormat();
        if (ppq <= 0)
            return notes;       // SMPTE-timed files have no bars to line up with

        for (int t = 0; t < source.getNumTracks(); ++t)
        {
            // only note-ons and note-offs: everything else in the track is dropped here
            MidiMessageSequence seq;
            for (auto* e : *source.getTrack (t))
                if (e->message.isNoteOnOrOff())
                    seq.addEvent (e->message);
            seq.updateMatchedPairs();

            for (auto* e : seq)
            {
                if (! e->message.isNoteOn())
                    continue;
                const double start = e->message.getTimeStamp() / ppq;
                const double end = e->noteOffObject != nullptr ? e->noteOffObject->message.getTimeStamp() / ppq : start + 0.25;
                notes.push_back ({ start, jmax (1.0 / 64.0, end - start), e->message.getNoteNumber(), e->message.getVelocity() });
            }
        }

        std::sort (notes.begin(), notes.end(), [] (const auto& a, const auto& b) { return a.start < b.start; });
        return notes;
    }

    ImportResult notesToPattern (const std::vector<ImportedNote>& notes, const ImportOptions& o)
    {
        ImportResult r;
        const double step = jmax (1.0 / 64.0, o.stepLength);
        const double origin = (jmax (1, o.startBar) - 1) * 4.0;
        constexpr int maxSteps = kMaxSteps;
        const int root = o.baseNote + jlimit (0, 11, o.key);

        // which note owns each step: the top (or bottom) of the notes starting there
        std::array<int, maxSteps> owner;
        owner.fill (-1);
        int lastStepUsed = -1;
        for (int i = 0; i < (int) notes.size(); ++i)
        {
            const auto k = (int) std::lround ((notes[(size_t) i].start - origin) / step);
            if (k < 0 || k >= maxSteps)
                continue;
            auto& cur = owner[(size_t) k];
            if (cur < 0 || (o.topNote ? notes[(size_t) i].note > notes[(size_t) cur].note
                                      : notes[(size_t) i].note < notes[(size_t) cur].note))
                cur = i;
        }

        // A note that rounded onto a step another note already has (played a little early or late,
        // not part of a chord) used to be lost. Keep both, in the order they were played: one stays on
        // the step and the other goes to the step next to it, if no note landed there (whichever way
        // moves them least). Steps without such a clash are exactly as before.
        for (int i = 0; i < (int) notes.size(); ++i)
        {
            const auto& n = notes[(size_t) i];
            const auto k = (int) std::lround ((n.start - origin) / step);
            if (k < 0 || k >= maxSteps || std::find (owner.begin(), owner.end(), i) != owner.end())
                continue;                                   // outside the steps, or already placed

            const int other = owner[(size_t) k];
            if (other < 0 || std::abs (notes[(size_t) other].start - n.start) < step / 3.0)
                continue;                                   // a chord: one note per step

            const int early = n.start < notes[(size_t) other].start ? i : other;
            const int late = early == i ? other : i;
            const double pe = (notes[(size_t) early].start - origin) / step, pl = (notes[(size_t) late].start - origin) / step;
            const bool laterFree = k + 1 < maxSteps && owner[(size_t) k + 1] < 0;
            const bool earlierFree = k - 1 >= 0 && owner[(size_t) k - 1] < 0;
            const double moveLate = laterFree ? std::abs (pe - k) + std::abs (pl - (k + 1)) : 1.0e9;
            const double moveEarly = earlierFree ? std::abs (pe - (k - 1)) + std::abs (pl - k) : 1.0e9;

            if (moveLate <= moveEarly && laterFree)
            {
                owner[(size_t) k] = early;
                owner[(size_t) k + 1] = late;
            }
            else if (earlierFree)
            {
                owner[(size_t) k - 1] = early;
                owner[(size_t) k] = late;
            }
        }

        auto toTranspose = [&] (int note)
        {
            int semis = note - root;
            while (semis > 24) semis -= 12;     // fold far notes by octaves into range
            while (semis < -24) semis += 12;
            if (! o.scaleSteps)
                return o.snapToScale ? arp::quantiseTranspose (semis, o.key, o.scale) : semis;
            return jlimit (-24, 24, arp::semitonesToScaleSteps (semis, o.scale));
        };

        auto& steps = r.pattern.steps;
        for (auto& s : steps)
            s.volume = 0;                       // rests unless a note lands there

        for (int k = 0; k < maxSteps; ++k)
        {
            const int i = owner[(size_t) k];
            if (i < 0)
                continue;

            const auto& n = notes[(size_t) i];
            ++r.notesUsed;
            auto& s = steps[(size_t) k];
            s.transpose = toTranspose (n.note);
            auto& count = r.pattern.notePool[(size_t) (s.transpose + 24)];
            count = (std::uint8_t) jmin (255, count + 1);
            s.velocity = jlimit (1, 127, n.velocity);
            s.volume = 127;
            lastStepUsed = jmax (lastStepUsed, k);

            // a long note: hold it with ties over the following empty steps
            const auto span = jmax (1, (int) std::lround (n.length / step));
            for (int j = k + 1; j < jmin (maxSteps, k + span); ++j)
            {
                if (owner[(size_t) j] >= 0)
                    break;                      // the next note starts here
                steps[(size_t) j - 1].tie = true;
                auto& held = steps[(size_t) j];
                held.transpose = s.transpose;
                held.velocity = s.velocity;
                held.volume = 127;
                lastStepUsed = jmax (lastStepUsed, j);
            }
        }

        // length: 16 or 32 steps; End when the content is shorter (rounded up to a bar)
        const int stepsPerBar = jmax (1, (int) std::lround (4.0 / step));
        int used = lastStepUsed + 1;
        if (used > 0 && stepsPerBar <= maxSteps)
            used = ((used + stepsPerBar - 1) / stepsPerBar) * stepsPerBar;
        used = jlimit (1, maxSteps, used);
        r.length = used > 16 ? 32 : 16;
        r.end = used < r.length ? used : 0;

        r.pattern.scaleSteps = o.scaleSteps;
        r.pattern.key = o.key;
        r.pattern.scale = o.scale;
        return r;
    }
}
