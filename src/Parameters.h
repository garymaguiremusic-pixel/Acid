#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>
#include <vector>

//==============================================================================
// Parameter IDs, choice lists, the knob -> real-unit mappings and the hover-help
// text. Every knob moves in steps of 1.0 (mouse wheel: one step per notch); where
// a real unit fits whole steps (semitones, dB, %, ms) the parameter uses that unit
// directly, otherwise it runs 0..100 and is mapped to real units here.
// IDs are stable: never rename one once it has shipped.
//==============================================================================
namespace acid
{
inline constexpr int kStateVersion = 3;   // 2: ArpSampler sequencer; 3: Cutoff, Cut Low, Delay Low / High Cut in Hz

/** The note a sequence's transpose 0 plays from (the Key's root in this octave): C1 = 36, bass register. */
inline constexpr int kSequenceBaseNote = 36;

namespace ParamIDs
{
    // Header
    inline constexpr auto tune        = "tune";
    inline constexpr auto volume      = "volume";
    inline constexpr auto echoOn      = "echoOn";
    inline constexpr auto tubeOn      = "tubeOn";

    // Synth page
    inline constexpr auto wave        = "wave";
    inline constexpr auto retro       = "retro";
    inline constexpr auto cutoffLow   = "cutoffLow";
    inline constexpr auto cutoff      = "cutoff";
    inline constexpr auto resonance   = "resonance";
    inline constexpr auto transistor  = "transistor";
    inline constexpr auto engine          = "engine";        // which synth the page shows and the sequencer plays
    inline constexpr auto oscAOctave      = "oscAOctave";
    inline constexpr auto oscBOctave      = "oscBOctave";

    // 303 page
    inline constexpr auto b3Wave          = "b3Wave";
    inline constexpr auto b3Tuning        = "b3Tuning";
    inline constexpr auto b3Octave        = "b3Octave";
    inline constexpr auto b3Cutoff        = "b3Cutoff";
    inline constexpr auto b3Resonance     = "b3Resonance";
    inline constexpr auto b3EnvMod        = "b3EnvMod";
    inline constexpr auto b3Decay         = "b3Decay";
    inline constexpr auto b3Accent        = "b3Accent";
    inline constexpr auto b3Volume        = "b3Volume";
    inline constexpr auto b3Model         = "b3Model";
    inline constexpr auto b3AccurateGate  = "b3AccurateGate";
    inline constexpr auto b3VcaNoise      = "b3VcaNoise";
    inline constexpr auto b3Vibrato       = "b3Vibrato";
    inline constexpr auto b3PhaseReset    = "b3PhaseReset";
    inline constexpr auto b3SlowSlide     = "b3SlowSlide";
    inline constexpr auto b3WeakAccentCap = "b3WeakAccentCap";
    inline constexpr auto b3InfSustain    = "b3InfSustain";
    inline constexpr auto b3SoftAccent    = "b3SoftAccent";
    inline constexpr auto b3AmpAttack     = "b3AmpAttack";     // the 303's own amp envelope
    inline constexpr auto b3AmpDecay      = "b3AmpDecay";
    inline constexpr auto b3AmpRelease    = "b3AmpRelease";
    inline constexpr auto b3AmpSync       = "b3AmpSync";
    inline constexpr auto b3VelLink       = "b3VelLink";      // the sequencer's Velocity lane plays the 303

    // Distortion page
    inline constexpr auto distPageOn      = "distPageOn";     // the header's Dist button
    inline constexpr auto distOn          = "distOn";         // the Saturate / Foldback section
    inline constexpr auto distMode        = "distMode";
    inline constexpr auto distDrive       = "distDrive";
    inline constexpr auto distAmount      = "distAmount";
    // the six distortion modes, each on / off (any number at once)
    inline constexpr auto distSaturate    = "distSaturate";
    inline constexpr auto distFoldback    = "distFoldback";
    inline constexpr auto distOverdrive   = "distOverdrive";
    inline constexpr auto distFuzz        = "distFuzz";
    inline constexpr auto distScream      = "distScream";
    inline constexpr auto distRectify     = "distRectify";
    inline const char* const distModeIds[] = { distSaturate, distFoldback, distOverdrive, distFuzz, distScream, distRectify };

    inline constexpr auto filterMode      = "filterMode";
    inline constexpr auto oscALock        = "oscALock";
    inline constexpr auto oscBLock        = "oscBLock";
    inline constexpr auto oscSync         = "oscSync";        // hard sync: Osc B restarts with Osc A
    inline constexpr auto oscSyncPitch    = "oscSyncPitch";   // Hard Sync: how far Osc A sweeps above the note
    inline constexpr auto fmAmount        = "fmAmount";
    inline constexpr auto fmDirection     = "fmDirection";    // A > B or B > A
    inline constexpr auto filterVelLink   = "filterVelLink";
    inline constexpr auto filterVelAmount = "filterVelAmount";
    inline constexpr auto bass        = "bass";
    inline constexpr auto treble      = "treble";
    inline constexpr auto drive       = "drive";
    inline constexpr auto envMod      = "envMod";
    inline constexpr auto decay       = "decay";
    inline constexpr auto accent      = "accent";
    inline constexpr auto envAttack   = "envAttack";
    inline constexpr auto accDecay    = "accDecay";
    inline constexpr auto accSweep    = "accSweep";

    // Sequencer (the ArpSampler arpeggiator + step sequencer). Glide = slideTime.
    inline constexpr auto slideTime   = "slideTime";
    inline constexpr auto arpOn       = "arpOn";
    inline constexpr auto arpMode     = "arpMode";
    inline constexpr auto arpRate     = "arpRate";
    inline constexpr auto arpRateType = "arpRateType";
    inline constexpr auto arpGate     = "arpGate";
    inline constexpr auto arpSwing    = "arpSwing";
    inline constexpr auto arpOctave   = "arpOctave";
    inline constexpr auto velMode     = "velMode";
    inline constexpr auto bpm         = "bpm";
    inline constexpr auto autoPlay    = "autoPlay";
    inline constexpr auto midiOut     = "midiOut";
    inline constexpr auto seqLength   = "seqLength";
    inline constexpr auto seqEnd      = "seqEnd";
    inline constexpr auto scaleKey    = "scaleKey";
    inline constexpr auto scale       = "scale";
    inline constexpr auto scaleLock   = "scaleLock";

    // Randomiser (saved, not automatable)
    inline constexpr auto rndDensity     = "rndDensity";
    inline constexpr auto rndRange       = "rndRange";
    inline constexpr auto rndVelMin      = "rndVelMin";
    inline constexpr auto rndVelMax      = "rndVelMax";
    inline constexpr auto rndRatchet     = "rndRatchet";
    inline constexpr auto rndAccent      = "rndAccent";
    inline constexpr auto rndGlide       = "rndGlide";
    inline constexpr auto rndLockGlide   = "rndLockGlide";
    inline constexpr auto rndLockPitch   = "rndLockPitch";
    inline constexpr auto rndLockVel     = "rndLockVel";
    inline constexpr auto rndLockRest    = "rndLockRest";
    inline constexpr auto rndLockRatchet = "rndLockRatchet";
    inline constexpr auto rndLockAccent  = "rndLockAccent";
    inline constexpr auto rndFileNotes   = "rndFileNotes";

    // MIDI export (saved, not automatable)
    inline constexpr auto exportBars  = "exportBars";
    inline constexpr auto exportRoot  = "exportRoot";
    inline constexpr auto exportChord = "exportChord";

    // Layers page
    inline constexpr auto oscBLevel   = "oscBLevel";
    inline constexpr auto oscBTune    = "oscBTune";
    inline constexpr auto oscBDetune  = "oscBDetune";
    inline constexpr auto unisonVoices = "unisonVoices";
    inline constexpr auto unisonSpread = "unisonSpread";
    inline constexpr auto unisonFree  = "unisonFree";
    inline constexpr auto ampAttack   = "ampAttack";
    inline constexpr auto ampDecay    = "ampDecay";
    inline constexpr auto ampRelease  = "ampRelease";
    inline constexpr auto ampSync     = "ampSync";       // the amp envelope times in note values
    inline constexpr auto echoSync    = "echoSync";
    inline constexpr auto echoTime    = "echoTime";
    inline constexpr auto echoMix     = "echoMix";
    inline constexpr auto echoFeedback = "echoFeedback";
    inline constexpr auto echoLowCut  = "echoLowCut";
    inline constexpr auto echoHighCut = "echoHighCut";
    inline constexpr auto echoWidth   = "echoWidth";


    // LFO 1 / 2: "lfo1Rate", "lfo2Rate" ...
    inline juce::String lfo (int index, const char* name) { return "lfo" + juce::String (index + 1) + name; }

    // Tube page
    inline constexpr auto tubeEdgeLow  = "tubeEdgeLow";
    inline constexpr auto tubeXover1   = "tubeXover1";
    inline constexpr auto tubeXover2   = "tubeXover2";
    inline constexpr auto tubeEdgeHigh = "tubeEdgeHigh";
    inline constexpr auto tubeResonator = "tubeResonator";
    inline constexpr auto tubeCharacter = "tubeCharacter";
    inline constexpr auto tubeInput    = "tubeInput";
    inline constexpr auto tubeOutput   = "tubeOutput";
    inline constexpr auto tubeMix      = "tubeMix";
    inline constexpr auto tubeDrive    = "tubeDrive";      // the single tube's drive (one band)

    // Bands 0..2: "tubeLowDrive", "tubeMidGain", "tubeHighSolo" ...
    inline juce::String band (int index, const char* name)
    {
        static const char* names[] = { "Low", "Mid", "High" };
        return juce::String ("tube") + names[index] + name;
    }
}

//==============================================================================
// Choice lists
inline juce::StringArray waveNames()       { return { "Saw", "Square" }; }
inline juce::StringArray filterModeNames() { return { "LP", "BP", "HP" }; }
inline juce::StringArray engineNames()     { return { "ACID", "303" }; }
inline juce::StringArray b3WaveNames()     { return { "SAW", "SQR" }; }
inline juce::StringArray b3ModelNames()    { return { "Classic", "Stable", "Drift" }; }
inline juce::StringArray distModeNames()   { return { "SATURATE", "FOLDBACK", "OVERDRIVE", "FUZZ", "SCREAM", "RECTIFY" }; }
inline juce::StringArray fmDirectionNames() { return { "1 > 2", "2 > 1" }; }
inline juce::StringArray lfoShapeNames()   { return { "Sine", "Triangle", "Saw Up", "Saw Down", "Square", "S&H", "Smooth Random" }; }
inline juce::StringArray lfoShapeShort()   { return { "SINE", "TRI", "SAW UP", "SAW DN", "SQUARE", "S&H", "SMOOTH" }; }
inline juce::StringArray lfoRetrigNames()  { return { "Free", "Note", "Bar" }; }
inline juce::StringArray lfoDestNames()    { return { "Cutoff", "Pitch", "Wave", "Reso", "Volume" }; }

enum LfoDest { DestCutoff = 0, DestPitch, DestWave, DestReso, DestVolume, NumLfoDests };

//==============================================================================
// Knob -> real units
inline float expMap (float v, float lo, float hi)   { return lo * std::pow (hi / lo, juce::jlimit (0.0f, 100.0f, v) / 100.0f); }
inline float norm100 (float v)                      { return juce::jlimit (0.0f, 100.0f, v) / 100.0f; }

inline float cutoffToHz (float v)       { return expMap (v, 20.0f, 20000.0f); }
inline float decayToMs (float v)        { return expMap (v, 30.0f, 3000.0f); }
inline float envAttackToMs (float v)    { return expMap (v, 0.3f, 30.0f); }
inline float ampAttackToMs (float v)    { return expMap (v, 1.0f, 100.0f); }    // 1 ms and up: no click as a note starts
inline bool  ampDecayHolds (float v)    { return v >= 99.5f; }
inline float ampDecayToMs (float v)     { return expMap (v, 10.0f, 10000.0f); }
inline float ampReleaseToMs (float v)   { return expMap (v, 10.0f, 1000.0f); }  // 10 ms and up: no click as a note ends
inline float echoFreeMs (float v)       { return expMap (v, 10.0f, 2000.0f); }
inline float echoFeedbackGain (float v) { return norm100 (v) * 0.95f; }
inline float echoLowCutHz (float v)     { return expMap (v, 20.0f, 2000.0f); }
inline float echoHighCutHz (float v)    { return expMap (v, 1000.0f, 20000.0f); }

/** Knobs whose parameter is in Hz (exact values; the knob still moves in 100 musical steps). */
inline bool isHzKnob (const juce::String& id)
{
    return id == ParamIDs::cutoff || id == ParamIDs::cutoffLow || id == ParamIDs::echoLowCut || id == ParamIDs::echoHighCut;
}
inline juce::Range<float> hzKnobRange (const juce::String& id)
{
    if (id == ParamIDs::echoLowCut)  return { 20.0f, 2000.0f };
    if (id == ParamIDs::echoHighCut) return { 1000.0f, 20000.0f };
    return { 20.0f, 20000.0f };
}
/** Older presets and sessions stored these as knob positions (0 - 100): to Hz. */
inline float legacyKnobToHz (const juce::String& id, float knob)
{
    const auto r = hzKnobRange (id);
    return std::round (expMap (knob, r.getStart(), r.getEnd()));
}
inline float lfoFreeHz (float v)        { return expMap (v, 0.05f, 20.0f); }

/** Tempo divisions: name and length in quarter notes. */
struct Division { const char* name; double beats; };
const std::vector<Division>& echoDivisions();
const std::vector<Division>& lfoDivisions();
const std::vector<Division>& ampDivisions();
int divisionIndex (float knobValue, int numDivisions);
float knobForDivision (int index, int numDivisions);

juce::String formatHz (float hz);
juce::String formatMs (float ms);
juce::String formatNumber (double v);

//==============================================================================
/** Display text (with units) for a parameter value, used by the host, the knobs'
    hover text and the help display. */
juce::String describeParameter (const juce::String& id, float value);

/** The other way: text a host or the user typed ("500 Hz", "1.2k", "250 ms", "0.5 s") to the
    parameter's own value. Knobs shown in Hz or ms take the typed Hz or ms (the nearest step). */
float parseParameter (const juce::String& id, const juce::String& text);

/** One-line explanation of a parameter for the hover help. */
juce::String helpForParameter (const juce::String& id);

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
} // namespace acid
