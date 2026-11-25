#pragma once

#include "Step.h"
#include <string>

namespace acid
{
/** Text form of a pattern for saving: "len:" then 6 characters per step
    (note hex, octave+2, accent, slide, ratchet, N/R/T). */
std::string patternToString (const Pattern&);
bool patternFromString (const std::string&, Pattern& out);

/** MIDI note of a step at the given transpose (Voice base note C2 = 36). */
int stepMidiNote (const Step&, int transpose) noexcept;

/** Pattern for a fresh / cleared slot: 16 steps of C, no accents or slides. */
Pattern makeDefaultPattern();
} // namespace acid
