#include <catch2/catch_test_macros.hpp>
#include "seq/Sequencer.h"
#include "seq/Pattern.h"
#include "seq/PatternTools.h"
#include <vector>

using namespace acid;

namespace
{
    struct Timed { int64_t sample; SeqEvent ev; };

    /** Runs the sequencer against a simulated host playhead. */
    std::vector<Timed> run (Sequencer& seq, const Pattern& p, double bpm, double sr, int64_t totalSamples,
                            int blockSize, double startPpq = 0.0, int keyTranspose = 0)
    {
        std::vector<Timed> events;
        SeqEvent buf[Sequencer::kMaxEvents];
        const double ppqPerSample = bpm / 60.0 / sr;
        int64_t pos = 0;
        while (pos < totalSamples)
        {
            const int n = (int) std::min<int64_t> (blockSize, totalSamples - pos);
            SeqTransport tr { true, startPpq + (double) pos * ppqPerSample, ppqPerSample };
            const int count = seq.process (p, tr, n, keyTranspose, buf);
            for (int i = 0; i < count; ++i)
                events.push_back ({ pos + buf[i].offset, buf[i] });
            pos += n;
        }
        return events;
    }

    std::vector<Timed> ons (const std::vector<Timed>& ev)
    {
        std::vector<Timed> r;
        for (auto& e : ev) if (e.ev.type == SeqEvent::NoteOn) r.push_back (e);
        return r;
    }

    Pattern plainPattern (int length = 16)
    {
        Pattern p;
        p.length = length;
        for (int i = 0; i < kMaxSteps; ++i)
            p.steps[(size_t) i].note = (uint8_t) (i % 12);
        return p;
    }
}

TEST_CASE ("Steps land on exact samples at any block size", "[seq]")
{
    const double sr = 48000.0, bpm = 120.0;      // 16th = 6000 samples
    for (int block : { 1, 17, 64, 512, 2048 })
    {
        Sequencer seq;
        const auto ev = ons (run (seq, plainPattern(), bpm, sr, 6000 * 16, block));
        REQUIRE (ev.size() == 16);
        for (size_t i = 0; i < ev.size(); ++i)
        {
            REQUIRE (ev[i].sample == (int64_t) i * 6000);
            REQUIRE (ev[i].ev.note == 36 + (int) (i % 12));
            REQUIRE_FALSE (ev[i].ev.legato);
        }
    }
}

TEST_CASE ("Gate closes after the gate fraction of the step", "[seq]")
{
    Sequencer seq;
    SeqSettings s;
    s.gate = 0.5f;
    seq.setSettings (s);
    const auto ev = run (seq, plainPattern(), 120.0, 48000.0, 6000 * 2, 64);
    REQUIRE (ev.size() >= 3);
    REQUIRE (ev[0].ev.type == SeqEvent::NoteOn);
    REQUIRE (ev[1].ev.type == SeqEvent::NoteOff);
    REQUIRE (ev[1].sample == 3000);
}

TEST_CASE ("Slide holds the gate and glides into the next step", "[seq]")
{
    Pattern p = plainPattern();
    p.steps[1].slide = true;    // step 2 slides into step 3
    Sequencer seq;
    const auto ev = run (seq, p, 120.0, 48000.0, 6000 * 4, 128);

    // step 2 on at 6000, no off until step 3's legato note-on at 12000
    bool sawOffBetween = false, sawLegato = false;
    for (auto& e : ev)
    {
        if (e.ev.type == SeqEvent::NoteOff && e.sample > 6000 && e.sample <= 12000) sawOffBetween = true;
        if (e.ev.type == SeqEvent::NoteOn && e.sample == 12000) sawLegato = e.ev.legato;
    }
    REQUIRE_FALSE (sawOffBetween);
    REQUIRE (sawLegato);
}

TEST_CASE ("Rest opens no gate, Tie extends the previous note", "[seq]")
{
    Pattern p = plainPattern();
    p.steps[1].gate = Gate::Rest;
    p.steps[3].gate = Gate::Tie;    // step 3 (index 2) is held through step 4
    Sequencer seq;
    const auto ev = run (seq, p, 120.0, 48000.0, 6000 * 5, 256);

    const auto on = ons (ev);
    REQUIRE (on[0].sample == 0);
    REQUIRE (on[1].sample == 12000);     // index 1 rested, index 3 tied: next on is index 4
    REQUIRE (on[2].sample == 24000);

    // the tied note closes half way through the tie step
    bool offAt21000 = false;
    for (auto& e : ev)
        if (e.ev.type == SeqEvent::NoteOff && e.sample == 21000) offAt21000 = true;
    REQUIRE (offAt21000);
}

TEST_CASE ("Accent is reported on accented steps, including slid notes", "[seq]")
{
    Pattern p = plainPattern();
    p.steps[0].slide = true;
    p.steps[1].accent = true;
    Sequencer seq;
    const auto on = ons (run (seq, p, 120.0, 48000.0, 6000 * 2, 512));
    REQUIRE (on.size() == 2);
    REQUIRE (on[1].ev.accent);
    REQUIRE (on[1].ev.legato);
}

TEST_CASE ("Ratchet subdivides the step", "[seq]")
{
    Pattern p = plainPattern();
    p.steps[0].ratchet = 3;     // four hits
    Sequencer seq;
    const auto on = ons (run (seq, p, 120.0, 48000.0, 6000, 100));
    REQUIRE (on.size() == 4);
    for (int i = 0; i < 4; ++i)
        REQUIRE (on[(size_t) i].sample == i * 1500);
}

TEST_CASE ("Swing delays the off-beat steps", "[seq]")
{
    Sequencer seq;
    SeqSettings s;
    s.swing = 0.75f;            // off-beats on the dotted position: +3000 samples
    seq.setSettings (s);
    const auto on = ons (run (seq, plainPattern(), 120.0, 48000.0, 24000, 333));
    REQUIRE (on[0].sample == 0);
    REQUIRE (on[1].sample == 9000);
    REQUIRE (on[2].sample == 12000);
    REQUIRE (on[3].sample == 21000);
}

TEST_CASE ("Directions", "[seq]")
{
    auto indices = [] (int direction, int length, int steps)
    {
        Sequencer seq;
        SeqSettings s;
        s.direction = direction;
        seq.setSettings (s);
        const auto on = ons (run (seq, plainPattern (length), 120.0, 48000.0, 6000 * steps, 512));
        std::vector<int> r;
        for (auto& e : on) r.push_back (e.ev.note - 36);
        return r;
    };

    REQUIRE (indices (1, 4, 6) == std::vector<int> { 3, 2, 1, 0, 3, 2 });
    REQUIRE (indices (2, 4, 8) == std::vector<int> { 0, 1, 2, 3, 2, 1, 0, 1 });
    for (int v : indices (3, 5, 32))
        REQUIRE ((v >= 0 && v < 5));
}

TEST_CASE ("Steps stay locked to the song position", "[seq]")
{
    Sequencer seq;
    // start in the middle of step 2: the first note is step 3, at its exact position
    const double startPpq = 0.25 + 0.1;
    const auto on = ons (run (seq, plainPattern(), 120.0, 48000.0, 12000, 256, startPpq));
    REQUIRE (on.size() >= 1);
    REQUIRE (on[0].ev.note == 36 + 2);
    REQUIRE (on[0].sample == 6000 - (int64_t) std::llround (0.1 / (120.0 / 60.0 / 48000.0)));
}

TEST_CASE ("Key transpose and pattern transpose add up", "[seq]")
{
    Sequencer seq;
    SeqSettings s;
    s.transpose = -12;
    seq.setSettings (s);
    const auto on = ons (run (seq, plainPattern(), 120.0, 48000.0, 6000, 512, 0.0, 7));
    REQUIRE (on[0].ev.note == 36 - 12 + 7);
}

TEST_CASE ("Stopping closes the gate", "[seq]")
{
    Sequencer seq;
    run (seq, plainPattern(), 120.0, 48000.0, 1000, 512);
    REQUIRE (seq.isGateOn());
    SeqEvent buf[Sequencer::kMaxEvents];
    SeqTransport stopped;
    REQUIRE (seq.process (plainPattern(), stopped, 512, 0, buf) == 1);
    REQUIRE (buf[0].type == SeqEvent::NoteOff);
}

TEST_CASE ("Pattern text round trip", "[seq]")
{
    Pattern p;
    PatternTools::randomise (p, PatternTools::RandomMode::All, 9, 1, 1234);
    p.length = 23;
    Pattern q;
    REQUIRE (patternFromString (patternToString (p), q));
    REQUIRE (p == q);
}

TEST_CASE ("Random respects the scale, rotate keeps the steps", "[seq]")
{
    Pattern p;
    PatternTools::randomise (p, PatternTools::RandomMode::Notes, 2, 5, 99);   // D minor pentatonic
    const std::vector<int> allowed { 2, 5, 7, 9, 0, 12 };
    for (auto& st : p.steps)
        REQUIRE (std::find (allowed.begin(), allowed.end(), (int) st.note) != allowed.end());

    Pattern r = p;
    r.length = 16;
    p.length = 16;
    PatternTools::rotate (r, 1);
    REQUIRE (r.steps[1] == p.steps[0]);
    REQUIRE (r.steps[0] == p.steps[15]);
    PatternTools::rotate (r, -1);
    REQUIRE (r == p);
}

TEST_CASE ("Undo and redo restore pattern snapshots", "[seq]")
{
    PatternBank bank {};
    PatternUndoStack undo;
    undo.push (3, bank[3]);
    bank[3].steps[0].accent = true;
    REQUIRE (undo.undoInto (bank));
    REQUIRE_FALSE (bank[3].steps[0].accent);
    REQUIRE (undo.redoInto (bank));
    REQUIRE (bank[3].steps[0].accent);
}
