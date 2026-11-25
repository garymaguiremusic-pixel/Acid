#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include "../Pattern.h"

namespace as::import
{
    /** A note found in a MIDI or audio file, timed in quarter notes from the start of the file. */
    struct ImportedNote
    {
        double start = 0.0, length = 0.25;
        int note = 60;
        int velocity = 100;
    };

    struct ImportOptions
    {
        double stepLength = 0.25;   // quarter notes per step (the arp rate)
        int startBar = 1;           // 1-based 4/4 bar of the file the steps start from
        bool topNote = true;        // chords: keep the top note (false: the bottom one)
        bool scaleSteps = false;    // Scale Lock on: numbers are notes of the scale
        bool snapToScale = false;   // without Scale Lock: still move out-of-key notes into the Key/Scale
        int key = 0, scale = 0;     // the Key and Scale (the Key's root in the base octave is transpose 0)
        int baseNote = 60;          // MIDI note of the Key's root that transpose 0 plays (Acid: 36, a bass register)
    };

    struct ImportResult
    {
        arp::Pattern pattern;
        int notesUsed = 0;          // notes that landed on a step
        int length = 16;            // 16 or 32 steps
        int end = 0;                // End step (0 = all), when the content is shorter than the length
    };

    /** Every note-on/note-off pair in the file, all tracks and channels. CCs, pitch bend,
        program changes, aftertouch and meta events are ignored. */
    std::vector<ImportedNote> readMidiNotes (const juce::MidiFile&);

    /**
        Puts notes onto the step grid: each note starts on its nearest step, a note longer
        than a step becomes a note plus ties, gaps become rests (Volume 0), and chords keep
        the top or bottom note. Pitches become transposes from the Key's root in octave 3
        (or scale steps with Scale Lock, out-of-key notes moving to the nearest scale note),
        folded by octaves into +/-24. Up to 32 steps from the start bar.
    */
    ImportResult notesToPattern (const std::vector<ImportedNote>&, const ImportOptions&);
}
