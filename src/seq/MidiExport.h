#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "Step.h"

namespace acid
{
/** One loop of the pattern as a MIDI file (960 PPQ, 120 BPM). Slides become
    overlapping notes, accents velocity 127, ties extend the previous note. */
juce::MidiFile patternToMidi (const Pattern&, int transpose, int rate, float gate);

/** Writes the pattern to a temporary .mid file (for drag-and-drop / export). */
juce::File writePatternMidiFile (const Pattern&, int transpose, int rate, float gate, const juce::String& name);

/** Reads notes from a MIDI file into a pattern on the given step grid. Overlapping
    notes become slides, velocity >= 100 becomes an accent, empty steps rests. */
bool midiToPattern (const juce::MidiFile&, int rate, Pattern& out);
} // namespace acid
