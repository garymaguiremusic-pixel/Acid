#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <vector>
#include "PatternImport.h"

namespace as::import
{
    /**
        Finds the notes of a single-note line (bass, acid, lead, vocal) in audio: pitch is
        followed frame by frame (YIN), a note starts where the sound starts, where the
        pitch moves to a new note, or where the level jumps (a repeated note), and ends
        where the sound stops. Velocity follows each note's peak level.
        Timing is converted to quarter notes at 'bpm', with the start of the audio on beat 1.
        Slow (about a second per minute of audio): run it off the audio and message threads.
    */
    std::vector<ImportedNote> detectNotes (const juce::AudioBuffer<float>&, double sampleRate, double bpm);

    /** Reads up to 'maxSeconds' of an audio file and runs detectNotes. Empty if it can't be read. */
    std::vector<ImportedNote> detectNotesInFile (const juce::File&, double bpm, double maxSeconds = 60.0);
}
