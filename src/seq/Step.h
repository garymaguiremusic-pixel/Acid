#pragma once

#include <array>
#include <cstdint>

namespace acid
{
inline constexpr int kMaxSteps    = 64;
inline constexpr int kNumPatterns = 16;
inline constexpr int kMaxRatchet  = 3;     // 0 = single hit, 3 = four hits

enum class Gate : uint8_t { Note = 0, Rest, Tie };

struct Step
{
    uint8_t note    = 0;          // 0..12 (C .. C')
    int8_t  octave  = 0;          // -2..+2 (Down / Up rows)
    bool    accent  = false;
    bool    slide   = false;      // glide into the next step, holding the gate
    uint8_t ratchet = 0;          // 0 = single hit
    Gate    gate    = Gate::Note;

    bool operator== (const Step&) const = default;
};

struct Pattern
{
    std::array<Step, kMaxSteps> steps {};
    int length = 16;

    bool operator== (const Pattern&) const = default;
};

using PatternBank = std::array<Pattern, kNumPatterns>;
} // namespace acid
