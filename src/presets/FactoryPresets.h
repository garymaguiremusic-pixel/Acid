#pragma once

#include "../seq/Step.h"
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
    std::vector<const char*> patterns;                   // in pattern notation, slot 1 upwards
};

const std::vector<FactoryPreset>& factoryPresets();

/** Pattern notation: one token per step, separated by spaces.
      C D# F ... B    note (C' = the high C)
      -               rest
      _               tie (extends the previous note)
    followed by any of:  ^ octave up (^^ two),  v octave down,  ! accent,  ~ slide,
                         x2 / x3 / x4 ratchet (hits per step).
    The number of tokens is the pattern length. */
Pattern patternFromNotation (const char* text);
} // namespace acid
