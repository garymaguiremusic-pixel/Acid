#include "MidiExport.h"
#include "Sequencer.h"
#include "Pattern.h"

namespace acid
{
using namespace juce;

namespace { constexpr int kPpq = 960; }

MidiFile patternToMidi (const Pattern& p, int transpose, int rate, float gate)
{
    MidiMessageSequence seq;
    seq.addEvent (MidiMessage::tempoMetaEvent (500000), 0.0);   // 120 BPM
    seq.addEvent (MidiMessage::timeSignatureMetaEvent (4, 4), 0.0);

    const double stepTicks = Sequencer::stepLengthPpq (rate) * kPpq;
    const int len = std::clamp (p.length, 1, kMaxSteps);

    int i = 0;
    while (i < len)
    {
        const auto& st = p.steps[(size_t) i];
        if (st.gate != Gate::Note)
        {
            ++i;
            continue;
        }

        const int note = stepMidiNote (st, transpose);
        const double start = i * stepTicks;
        int last = i;
        // ties extend the note; a slide makes it overlap the next one
        while (last + 1 < len && p.steps[(size_t) last + 1].gate == Gate::Tie)
            ++last;
        const bool slide = p.steps[(size_t) last].slide && last + 1 < len && p.steps[(size_t) last + 1].gate == Gate::Note;
        const double end = slide ? (last + 1) * stepTicks + stepTicks * 0.1
                                 : last * stepTicks + stepTicks * std::clamp ((double) gate, 0.05, 1.0);

        seq.addEvent (MidiMessage::noteOn (1, note, (uint8) (st.accent ? 127 : 100)), start);
        seq.addEvent (MidiMessage::noteOff (1, note), end);
        i = last + 1;
    }
    seq.updateMatchedPairs();

    MidiFile file;
    file.setTicksPerQuarterNote (kPpq);
    file.addTrack (seq);
    return file;
}

File writePatternMidiFile (const Pattern& p, int transpose, int rate, float gate, const String& name)
{
    auto file = File::getSpecialLocation (File::tempDirectory).getChildFile (File::createLegalFileName (name) + ".mid");
    file.deleteFile();
    FileOutputStream out (file);
    if (! out.openedOk())
        return {};
    patternToMidi (p, transpose, rate, gate).writeTo (out);
    out.flush();
    return file;
}

bool midiToPattern (const MidiFile& source, int rate, Pattern& out)
{
    const int tpq = source.getTimeFormat() > 0 ? source.getTimeFormat() : kPpq;
    const double stepTicks = Sequencer::stepLengthPpq (rate) * tpq;

    struct N { double start, end; int note, vel; };
    std::vector<N> notes;
    for (int t = 0; t < source.getNumTracks(); ++t)
    {
        MidiMessageSequence seq (*source.getTrack (t));
        seq.updateMatchedPairs();
        for (auto* e : seq)
            if (e->message.isNoteOn() && e->noteOffObject != nullptr)
                notes.push_back ({ e->message.getTimeStamp(), e->noteOffObject->message.getTimeStamp(),
                                   e->message.getNoteNumber(), e->message.getVelocity() });
    }
    if (notes.empty())
        return false;

    std::sort (notes.begin(), notes.end(), [] (const N& a, const N& b) { return a.start < b.start; });
    const double first = std::floor (notes.front().start / stepTicks) * stepTicks;

    Pattern p;
    for (auto& st : p.steps) st.gate = Gate::Rest;
    int length = 1;
    std::array<bool, kMaxSteps> used {};

    for (auto& n : notes)
    {
        const int step = (int) std::llround ((n.start - first) / stepTicks);
        if (step < 0 || step >= kMaxSteps || used[(size_t) step])
            continue;
        used[(size_t) step] = true;

        auto& st = p.steps[(size_t) step];
        const int rel = n.note - 36;
        int octave = (int) std::floor (rel / 12.0);
        int pitch = rel - octave * 12;
        if (octave > 2) { pitch = 12; octave = 2; }   // clamp very high notes to the top C
        st.octave = (int8_t) std::clamp (octave, -2, 2);
        st.note = (uint8_t) std::clamp (pitch, 0, 12);
        st.accent = n.vel >= 100;
        st.gate = Gate::Note;
        // overlaps the next step's start -> slide
        st.slide = n.end > (step + 1) * stepTicks + first + stepTicks * 0.02;
        length = std::max (length, step + 1);
    }

    // round the length up to a whole bar when it is close
    const int bar = (int) std::llround (4.0 / Sequencer::stepLengthPpq (rate));
    if (bar > 0) length = std::min (kMaxSteps, ((length + bar - 1) / bar) * bar);
    p.length = length;
    out = p;
    return true;
}
} // namespace acid
