#pragma once

#include "../arp/Pattern.h"
#include <string>
#include <utility>
#include <vector>

namespace acid
{
struct FactoryPreset
{
    const char* name;
    const char* bank;
    std::vector<std::pair<std::string, float>> params;   // everything else stays at its default
    const char* pattern;                                 // in pattern notation (below)
};

/** A step pattern for the sequencer, plus how many steps it has. */
struct ConvertedPattern
{
    as::arp::Pattern pattern;
    int length = 16;      // 1..32
};

const std::vector<FactoryPreset>& factoryPresets();

/** Acid pattern notation (one token per step, separated by spaces), turned into sequencer
    steps: the note becomes the step's transpose from the Key's root, a slide becomes a Glide
    (it slides into the next step), a tie holds the previous note, a rest is Volume 0.
      C D# F ... B    note (C' = the high C)
      -               rest
      _               tie (extends the previous note)
    followed by any of:  ^ octave up (^^ two),  v octave down,  ! accent,  ~ slide,
                         x2 / x3 / x4 ratchet (hits per step).
    The number of tokens is the pattern length. */
ConvertedPattern patternFromNotation (const char* text);

/** Patterns saved by the first version of Acid ("len:" + 6 characters per step), converted
    the same way, so older projects keep their line. */
ConvertedPattern patternFromLegacyString (const std::string& text);
} // namespace acid
