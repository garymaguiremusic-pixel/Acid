#pragma once

#include "Step.h"
#include "../dsp/DspCommon.h"

namespace acid
{
struct SeqEvent
{
    enum Type : uint8_t { NoteOff = 0, NoteOn = 1 };
    int offset = 0;         // sample offset within the block
    Type type = NoteOn;
    int note = 60;
    bool accent = false;
    bool legato = false;    // slide into this note (no envelope retrigger)
};

struct SeqSettings
{
    int   rate      = 1;     // 0 = 1/8, 1 = 1/16, 2 = 1/16T, 3 = 1/32
    float swing     = 0.0f;  // 0..0.75 (0.75: off-beat steps land on the dotted position)
    int   direction = 0;     // 0 Forward, 1 Reverse, 2 Ping-pong, 3 Random
    int   transpose = 0;     // semitones
    float gate      = 0.5f;  // 0.05..1, fraction of the step the gate stays open
};

struct SeqTransport
{
    bool   running = false;
    double ppqStart = 0.0;        // position (in quarter notes) of the block's first sample
    double ppqPerSample = 0.0;
};

/** Step sequencer with 303-style rules:
     - slide on step N holds the gate into step N+1, which glides (no retrigger)
     - a normal step closes its gate after Gate x the step length
     - Rest opens no gate (and closes a held one), Tie extends the previous note
     - accent is decided at note-on (also on slid notes, without retriggering)
     - ratchet subdivides the step into ratchet+1 hits
    Steps sit on an absolute grid derived from the song position, so the pattern stays
    locked to the host's bars. All events are sample-accurate within the block. */
class Sequencer
{
public:
    static constexpr int kMaxEvents = 256;

    static double stepLengthPpq (int rate) noexcept;

    void reset() noexcept;
    void setSettings (const SeqSettings& s) noexcept { set = s; }

    /** Generates the block's events (sorted by offset) into out; returns how many.
        keyTranspose is added to every note (Key mode). */
    int process (const Pattern& pattern, const SeqTransport& transport, int numSamples,
                 int keyTranspose, SeqEvent* out) noexcept;

    /** Closes a sounding note immediately (returns 1 if an event was written). */
    int stop (SeqEvent* out) noexcept;

    int getCurrentStep() const noexcept   { return currentStep; }
    bool isGateOn() const noexcept        { return gateOn; }

private:
    struct Pending
    {
        double ppq;
        SeqEvent::Type type;
        int note;
        bool accent;
        uint32_t gen;     // for note-offs: only applies to this note generation
    };

    int indexForStep (int64_t n, int length) noexcept;
    void triggerStep (const Pattern&, int64_t n, double stepStart, double stepEnd, int keyTranspose);
    void addPending (const Pending&) noexcept;
    double boundary (int64_t n) const noexcept;   // ppq where absolute step n starts
    void pushRaw (double ppq, const SeqEvent& e) noexcept
    {
        if (numRaw < kMaxEvents)
            raw[numRaw++] = { ppq, e, orderCounter++ };
    }

    SeqSettings set;
    Pending pending[64];
    int numPending = 0;

    // events of the current block, before they get sample offsets
    struct Raw { double ppq; SeqEvent ev; int order; };
    Raw raw[kMaxEvents];
    int numRaw = 0, orderCounter = 0;

    bool gateOn = false, holdIntoNext = false, positionValid = false;
    double lastPpqEnd = 0.0;
    int64_t lastStepTriggered = -1;
    int currentStep = -1, soundingNote = -1;
    uint32_t noteGen = 0;
    bool randomNextValid = false;
    int randomNext = 0;
    int64_t randomNextFor = -1;
    FastRandom rng { 0xacedu };
};
} // namespace acid
