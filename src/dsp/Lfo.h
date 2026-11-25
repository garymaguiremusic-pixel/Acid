#pragma once

#include "DspCommon.h"

namespace acid
{
/** LFO with seven shapes, free or tempo-synced rate, and three retrigger modes:
    Free (runs on its own), Note (restarts on every new note), Bar (phase locked to
    the song position). Mirror turns the bipolar output into a unipolar 0..1 one. */
class Lfo
{
public:
    enum Shape  { Sine = 0, Triangle, SawUp, SawDown, Square, SampleHold, SmoothRandom, NumShapes };
    enum Retrig { RetrigFree = 0, RetrigNote, RetrigBar };

    void prepare (double sampleRate) noexcept { sr = sampleRate; phase = 0.0; }
    void reset() noexcept { phase = 0.0; }

    void setShape (int s) noexcept           { shape = std::clamp (s, 0, (int) NumShapes - 1); }
    void setPhaseOffset (float deg) noexcept { phaseOffset = deg / 360.0; }
    void setMirror (bool m) noexcept         { mirror = m; }
    void setRetrig (int r) noexcept          { retrig = r; }

    /** Call on every new (non-slid) note. */
    void noteTriggered() noexcept { if (retrig == RetrigNote) { phase = 0.0; newRandom(); } }

    /** Fills out[0..n) with the raw shape (-1..1, or 0..1 when mirrored).
        cyclesPerSecond: the rate. For Bar mode give cyclesPerBeat and the playhead
        position; with no valid position it falls back to free running. */
    void process (float* out, int numSamples, double cyclesPerSecond,
                  double cyclesPerBeat, bool positionValid, double ppqStart, double ppqPerSample) noexcept;

    double getPhase() const noexcept { return phase; }

    /** Shape value at a phase (0..1) for drawing. Random shapes use a fixed pattern. */
    static float shapeValue (int shape, double phase) noexcept;

private:
    float valueAt (double ph) noexcept;
    void newRandom() noexcept { prevRandom = nextRandom; nextRandom = rng.nextBipolar(); }

    double sr = 44100.0, phase = 0.0, phaseOffset = 0.0;
    int shape = Triangle, retrig = RetrigFree;
    bool mirror = false;
    float prevRandom = 0.0f, nextRandom = 0.0f;
    FastRandom rng { 0x1f0a9u };
};
} // namespace acid
