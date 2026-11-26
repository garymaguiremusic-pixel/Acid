// Tests for the ArpSampler sequencer as it runs in Acid (step timing, ties = glide,
// rests, accents, ratchets), the preset conversion, MIDI import and export.
#include <catch2/catch_test_macros.hpp>
#include "arp/ArpEngine.h"
#include "arp/Randomiser.h"
#include "arp/import/PatternImport.h"
#include "arp/export/MidiRenderer.h"
#include "presets/FactoryPresets.h"
#include <vector>

using namespace as;
using namespace as::arp;

namespace
{
    constexpr double kSr = 48000.0, kBpm = 120.0;     // a 16th = 6000 samples
    constexpr int kBase = 36;                           // Acid plays sequences from C1

    ArpSettings stepSettings()
    {
        ArpSettings s;
        s.mode = ArpMode::Step;
        s.rateIndex = 3;
        s.gate = 0.5f;
        s.autoPlay = true;
        s.autoNote = kBase;
        s.retrigger = true;
        s.sequenceLength = 16;
        return s;
    }

    struct Timed { juce::int64 sample; NoteEvent ev; };

    std::vector<Timed> run (const Pattern& p, ArpSettings s, juce::int64 samples, int block = 480)
    {
        ArpEngine arp;
        arp.prepare (kSr);
        arp.setSettings (s);
        arp.setPattern (p);
        std::vector<Timed> out;
        EventList list;
        juce::MidiBuffer midi;
        for (juce::int64 pos = 0; pos < samples; pos += block)
        {
            TransportInfo t;
            t.hostPlaying = true;
            t.bpm = kBpm;
            t.ppqAtBlockStart = (double) pos * kBpm / 60.0 / kSr;
            list.clear();
            arp.process (midi, block, t, list);
            for (auto& e : list)
                out.push_back ({ pos + e.offset, e });
        }
        return out;
    }

    std::vector<Timed> ofType (const std::vector<Timed>& v, NoteEvent::Type type)
    {
        std::vector<Timed> r;
        for (auto& e : v) if (e.ev.type == type) r.push_back (e);
        return r;
    }
}

TEST_CASE ("Auto Play plays the step pattern from the Key's root on the host grid", "[arp]")
{
    Pattern p;
    for (int i = 0; i < 16; ++i)
        p.steps[(size_t) i].transpose = i % 5;
    const auto starts = ofType (run (p, stepSettings(), 6000 * 16), NoteEvent::Type::Start);
    REQUIRE (starts.size() == 16);
    for (size_t i = 0; i < starts.size(); ++i)
    {
        REQUIRE (starts[i].sample == (juce::int64) i * 6000);
        REQUIRE (starts[i].ev.note == kBase + (int) i % 5);
    }
}

TEST_CASE ("A tie holds into the next step and changes pitch there (the glide)", "[arp]")
{
    Pattern p;
    p.steps[0].transpose = 0;
    p.steps[0].tie = true;
    p.steps[1].transpose = 7;
    const auto ev = run (p, stepSettings(), 6000 * 3);

    const auto changes = ofType (ev, NoteEvent::Type::Change);
    REQUIRE (changes.size() == 1);
    REQUIRE (changes[0].sample == 6000);
    REQUIRE (changes[0].ev.note == kBase + 7);

    // no stop of the first note before the change
    for (auto& e : ofType (ev, NoteEvent::Type::Stop))
        REQUIRE_FALSE ((e.sample > 0 && e.sample <= 6000 && e.ev.id == changes[0].ev.id));
}

TEST_CASE ("Rests are silent, accents are flagged, ratchets retrigger", "[arp]")
{
    Pattern p;
    p.steps[1].volume = 0;       // rest
    p.steps[2].accent = true;
    p.steps[3].ratchet = 3;
    const auto starts = ofType (run (p, stepSettings(), 6000 * 4), NoteEvent::Type::Start);

    std::vector<juce::int64> times;
    for (auto& s : starts) times.push_back (s.sample);
    REQUIRE (times == std::vector<juce::int64> { 0, 12000, 18000, 20000, 22000 });
    REQUIRE (starts[1].ev.accent);
    REQUIRE (starts[1].ev.velocity == 127);
}

TEST_CASE ("Factory notation becomes sequencer steps", "[presets]")
{
    // A, high C accented and slid, rest, D held by a tie, E an octave up with 2 ratchet hits
    const auto c = acid::patternFromNotation ("A C'!~ D - D~ _ E^x2");
    REQUIRE (c.length == 7);
    const auto& s = c.pattern.steps;
    REQUIRE (s[0].transpose == 9);
    REQUIRE (s[1].transpose == 12);
    REQUIRE (s[1].accent);
    REQUIRE (s[1].glide);                // slide -> Glide (slides into D)
    REQUIRE_FALSE (s[1].tie);
    REQUIRE (s[3].isRest());
    REQUIRE (s[4].glide);                // held into the next step (the _) by a same-pitch Glide
    REQUIRE_FALSE (s[4].tie);
    REQUIRE (s[5].transpose == s[4].transpose);
    REQUIRE (s[6].transpose == 16);
    REQUIRE (s[6].ratchet == 2);

    // every factory preset converts to a valid pattern
    for (const auto& preset : acid::factoryPresets())
    {
        const auto conv = acid::patternFromNotation (preset.pattern);
        REQUIRE (conv.length >= 1);
        REQUIRE (conv.length <= kMaxSteps);
        for (auto& st : conv.pattern.steps)
            REQUIRE ((st.transpose >= -24 && st.transpose <= 24));
    }
}

TEST_CASE ("Patterns saved by the first Acid version still load", "[presets]")
{
    // 16 steps: step 1 = A accent + slide, step 2 = rest, rest default (C)
    // 6 characters per step: note (hex), octave + 2, accent, slide, ratchet, N / R / T
    std::string legacy = "16:";
    legacy += "92110N";
    legacy += "02000R";
    for (int i = 2; i < 64; ++i) legacy += "02000N";
    const auto c = acid::patternFromLegacyString (legacy);
    REQUIRE (c.length == 16);
    REQUIRE (c.pattern.steps[0].transpose == 9);
    REQUIRE (c.pattern.steps[0].accent);
    REQUIRE_FALSE (c.pattern.steps[0].glide);   // its slide went into a rest
    REQUIRE (c.pattern.steps[1].isRest());
}

TEST_CASE ("Imported notes are placed relative to the bass octave", "[import]")
{
    std::vector<import::ImportedNote> notes { { 0.0, 0.25, kBase + 3, 100 }, { 0.25, 0.25, kBase + 10, 120 } };
    import::ImportOptions o;
    o.baseNote = kBase;
    const auto r = import::notesToPattern (notes, o);
    REQUIRE (r.notesUsed == 2);
    REQUIRE (r.pattern.steps[0].transpose == 3);
    REQUIRE (r.pattern.steps[1].transpose == 10);
}

TEST_CASE ("Drag MIDI renders the same notes the sequencer plays", "[export]")
{
    exporter::ExportRequest r;
    r.settings = stepSettings();
    r.settings.autoPlay = false;
    for (int i = 0; i < 16; ++i)
        r.pattern.steps[(size_t) i].transpose = i % 3;
    r.pattern.steps[4].volume = 0;
    r.input = { { kBase, 100 } };
    r.bars = 1;
    const auto file = exporter::renderPattern (r);

    int noteOns = 0;
    for (int t = 0; t < file.getNumTracks(); ++t)
        for (auto* e : *file.getTrack (t))
            if (e->message.isNoteOn())
                ++noteOns;
    REQUIRE (noteOns == 15);      // 16 steps minus the rest
}

TEST_CASE ("Randomise is repeatable per seed and keeps locked lanes", "[randomise]")
{
    Pattern current;
    current.steps[0].accent = true;
    RandomiseSettings s;
    s.accentProbability = 0.25f;
    s.lockAccents = true;
    const auto a = randomisePattern (current, s, 42);
    const auto b = randomisePattern (current, s, 42);
    REQUIRE (a == b);
    REQUIRE (a.steps[0].accent);
    for (size_t i = 1; i < a.steps.size(); ++i)
        REQUIRE_FALSE (a.steps[i].accent);
}

TEST_CASE ("Glide slides into the next step, Tie holds without sliding", "[arp]")
{
    Pattern p;
    p.steps[0].glide = true;
    p.steps[1].transpose = 5;
    p.steps[1].tie = true;
    p.steps[2].transpose = 9;
    const auto changes = ofType (run (p, stepSettings(), 6000 * 3), NoteEvent::Type::Change);
    REQUIRE (changes.size() == 2);
    REQUIRE (changes[0].sample == 6000);
    REQUIRE (changes[0].ev.glide);          // from the Glide step: slide
    REQUIRE (changes[1].sample == 12000);
    REQUIRE_FALSE (changes[1].ev.glide);    // from the Tie step: jump
}

TEST_CASE ("Randomise fills the Glide lane and its lock keeps it", "[randomise]")
{
    Pattern current;
    RandomiseSettings s;
    s.glideProbability = 1.0f;
    s.density = 1.0f;
    auto a = randomisePattern (current, s, 7);
    for (auto& st : a.steps) REQUIRE (st.glide);
    s.lockGlides = true;
    auto b = randomisePattern (current, s, 7);
    for (auto& st : b.steps) REQUIRE_FALSE (st.glide);
}
