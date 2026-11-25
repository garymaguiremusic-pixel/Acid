#pragma once

// The enums, limits and name lists the ArpSampler sequencer code uses (namespace "as"),
// so its files come across unchanged.

#include <juce_core/juce_core.h>

namespace as
{
    inline constexpr float kDefaultBpm = 120.0f;
    inline constexpr int kMaxSteps = 32;

    enum class ArpMode { Up, Down, UpDown, UpDown2, DownUp, DownUp2, Random, Ordered, Step, Chord };
    enum class RateType { Straight, Triplet, Dotted };
    enum class VelocityMode { Key, Hold, Step, StepKey, StepHold };

    inline juce::StringArray getArpModeNames()   { return { "Up", "Down", "Up/Down", "Up/Down2", "Down/Up", "Down/Up2", "Random", "Ordered", "Step", "Chord" }; }
    inline juce::StringArray getRateNames()      { return { "1/2", "1/4", "1/8", "1/16", "1/32" }; }
    inline juce::StringArray getRateTypeNames()  { return { "Straight", "Triplet", "Dotted" }; }
    inline juce::StringArray getVelModeNames()   { return { "Key", "Hold", "Step", "Step+Key", "Step+Hold" }; }
    inline juce::StringArray getSeqLengthNames() { return { "16", "32" }; }
    inline juce::StringArray getKeyNames()       { return { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }; }
    inline juce::StringArray getExportBarNames() { return { "1 bar", "2 bars", "4 bars", "8 bars" }; }
    inline juce::StringArray getExportChordNames() { return { "Major", "Minor", "Maj7", "Min7", "Dom7", "Sus4", "Octave", "Single" }; }

    inline juce::StringArray getSeqEndNames()
    {
        juce::StringArray s { "No" };
        for (int i = 1; i <= kMaxSteps; ++i)
            s.add (juce::String (i));
        return s;
    }

    inline juce::StringArray getScaleNames()
    {
        return { "Major", "Natural Minor", "Harmonic Minor", "Melodic Minor", "Dorian", "Phrygian",
                 "Lydian", "Mixolydian", "Locrian", "Major Pentatonic", "Minor Pentatonic", "Blues",
                 "Phrygian Dominant", "Hungarian Minor", "Whole Tone", "Japanese (In Sen)" };
    }

    /** MIDI note name with C3 = 60. */
    inline juce::String noteName (int note)
    {
        note = juce::jlimit (0, 127, note);
        return getKeyNames()[note % 12] + juce::String (note / 12 - 2);
    }
}
