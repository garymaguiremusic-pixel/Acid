#pragma once

#include "SampleData.h"
#include <vector>

namespace as::dsp
{
    struct PitchResult
    {
        bool pitched = false;
        float frequency = 0.0f;     // Hz
        float midiNote = 0.0f;      // fractional MIDI note (69 = A 440)
        float confidence = 0.0f;    // 0..1
    };

    /**
        Estimates a sample's pitch with the YIN algorithm over several windows after
        the attack. Drums, noise and silence come back as unpitched. Runs on the
        loader thread (it is far too slow for the audio thread).
    */
    PitchResult detectPitch (const SampleData&);
    PitchResult detectPitch (const float* mono, int numFrames, double sampleRate);

    /** YIN on one short window (for following a melody frame by frame). 'x' must hold
        window + rate/40 + 2 samples. 'confidence' is 1 - aperiodicity. */
    PitchResult detectFramePitch (const float* x, int window, double rate, std::vector<float>& scratch);

    /**
        Key lock: the Root note (and Fine correction in cents) that makes a sample with
        this pitch play in 'key' (0 = C .. 11 = B). The root is chosen so that playing
        the key's note in octave 3 (60 + key) shifts the sample by at most 6 semitones.
    */
    struct KeyLock
    {
        int rootNote = 60;
        int fineCents = 0;
    };
    KeyLock keyLockedRoot (float detectedMidiNote, int key) noexcept;
}
