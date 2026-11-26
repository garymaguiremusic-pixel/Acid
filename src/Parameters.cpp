#include "Parameters.h"
#include <map>
#include "arp/ArpParameters.h"

namespace acid
{
using namespace juce;

//==============================================================================
const std::vector<Division>& echoDivisions()
{
    static const std::vector<Division> d =
    {
        { "1/64", 1.0 / 16 }, { "1/32 T", 1.0 / 12 }, { "1/32", 1.0 / 8 }, { "1/16 T", 1.0 / 6 },
        { "1/32 .", 3.0 / 16 }, { "1/16", 1.0 / 4 }, { "1/8 T", 1.0 / 3 }, { "1/16 .", 3.0 / 8 },
        { "1/8", 1.0 / 2 }, { "1/4 T", 2.0 / 3 }, { "1/8 .", 3.0 / 4 }, { "1/4", 1.0 },
        { "1/2 T", 4.0 / 3 }, { "1/4 .", 3.0 / 2 }, { "1/2", 2.0 }, { "1/2 .", 3.0 }, { "1/1", 4.0 },
    };
    return d;
}

const std::vector<Division>& lfoDivisions()
{
    static const std::vector<Division> d =
    {
        { "8/1", 32.0 }, { "4/1", 16.0 }, { "2/1", 8.0 }, { "1/1", 4.0 }, { "1/2", 2.0 }, { "1/2 T", 4.0 / 3 },
        { "1/4 .", 1.5 }, { "1/4", 1.0 }, { "1/4 T", 2.0 / 3 }, { "1/8 .", 0.75 }, { "1/8", 0.5 },
        { "1/8 T", 1.0 / 3 }, { "1/16 .", 0.375 }, { "1/16", 0.25 }, { "1/16 T", 1.0 / 6 }, { "1/32", 0.125 },
    };
    return d;
}

const std::vector<Division>& ampDivisions()
{
    static const std::vector<Division> d =
    {
        { "1/64", 1.0 / 16 }, { "1/32 T", 1.0 / 12 }, { "1/32", 0.125 }, { "1/16 T", 1.0 / 6 },
        { "1/16", 0.25 }, { "1/16 .", 0.375 }, { "1/8 T", 1.0 / 3 }, { "1/8", 0.5 }, { "1/8 .", 0.75 },
        { "1/4 T", 2.0 / 3 }, { "1/4", 1.0 }, { "1/4 .", 1.5 }, { "1/2", 2.0 }, { "1/1", 4.0 },
    };
    return d;
}

int divisionIndex (float v, int n)    { return jlimit (0, n - 1, roundToInt (norm100 (v) * (float) (n - 1))); }
float knobForDivision (int i, int n)  { return std::round (100.0f * (float) i / (float) (n - 1)); }

String formatHz (float hz)
{
    if (hz >= 1000.0f) return String (hz / 1000.0f, hz >= 10000.0f ? 1 : 2) + " kHz";
    return String (roundToInt (hz)) + " Hz";
}

String formatMs (float ms)
{
    if (ms >= 1000.0f) return String (ms / 1000.0f, 2) + " s";
    if (ms < 10.0f)    return String (ms, 2) + " ms";
    if (ms < 100.0f) return String (ms, 1) + " ms";
    return String (roundToInt (ms)) + " ms";
}

String formatNumber (double v)
{
    const double r = std::round (v * 10.0) / 10.0;
    return std::abs (r - std::round (r)) < 0.05 ? String (roundToInt (r)) : String (r, 1);
}

//==============================================================================
namespace
{
    bool endsWith (const String& id, const char* s) { return id.endsWith (s); }
    String signedText (float v, const String& unit)  { return (v > 0.0f ? "+" : "") + formatNumber (v) + unit; }
}

float parseParameter (const String& id, const String& text)
{
    using namespace ParamIDs;
    const auto t = text.trim().toLowerCase();
    const float number = t.getFloatValue();

    // knobs that show Hz or ms: the typed Hz / ms back to the knob position (exponential map)
    auto inverse = [] (float x, float lo, float hi) { return jlimit (0.0f, 100.0f, 100.0f * std::log (jmax (lo, x) / lo) / std::log (hi / lo)); };
    auto hz = [&] { return number * ((t.contains ("k") && ! t.contains ("ms")) ? 1000.0f : 1.0f); };
    auto ms = [&] { return number * ((t.endsWith ("s") && ! t.endsWith ("ms")) ? 1000.0f : 1.0f); };

    if (isHzKnob (id))                     return hzKnobRange (id).clipValue (std::round (hz()));   // exact Hz
    if (id == decay || id == accDecay)     return inverse (ms(), 30.0f, 3000.0f);
    if (id == envAttack)                   return inverse (ms(), 0.3f, 30.0f);
    if (id == ampAttack || id == b3AmpAttack)   return inverse (ms(), 1.0f, 100.0f);
    if (id == ampRelease || id == b3AmpRelease) return inverse (ms(), 10.0f, 1000.0f);
    if (id == ampDecay || id == b3AmpDecay)     return t.contains ("hold") ? 100.0f : jmin (99.0f, inverse (ms(), 10.0f, 10000.0f));
    if (id == echoTime && ! t.contains ("/"))   return inverse (ms(), 10.0f, 2000.0f);
    if (id == echoFeedback)                return jlimit (0.0f, 100.0f, number / 0.95f);   // shown as % of the full feedback
    return number;
}

String describeParameter (const String& id, float v)
{
    using namespace ParamIDs;

    if (id == tune || id == oscBTune || id == b3Tuning) return signedText (v, " st");
    if (id == oscAOctave || id == oscBOctave || id == b3Octave) return "OCT " + String (v > 0.0f ? "+" : "") + String (roundToInt (v));
    if (id == volume || id == bass || id == treble || id == tubeInput || id == tubeOutput) return signedText (v, " dB");
    if (isHzKnob (id))                   return String (roundToInt (v)) + " Hz";     // exact, whole Hz
    if (id == decay || id == accDecay)   return formatMs (decayToMs (v));
    if (id == envAttack)                 return formatMs (envAttackToMs (v));
    if (id == arpGate)                   return formatNumber (v) + " %";
    if (id == arpSwing)                  return signedText (v, " %");
    if (id == slideTime)                 return formatNumber (v) + " ms";
    if (id == bpm)                       return formatNumber (v) + " BPM";
    if (id == arpOctave)                 return String (roundToInt (v)) + (roundToInt (v) == 1 ? " octave" : " octaves");
    if (id == rndRange)                  { const int r = roundToInt (v); return r % 12 == 0 && r > 0 ? "+/-" + String (r / 12) + " oct" : "+/-" + String (r) + " st"; }
    if (id == rndVelMin || id == rndVelMax) return String (roundToInt (v));
    if (id == exportRoot)                return v < 0.0f ? String ("Auto") : as::noteName (roundToInt (v));
    if (id == oscBDetune)                return signedText (v, " ct");
    if (id == unisonVoices)              return String (roundToInt (v)) + (roundToInt (v) == 1 ? " voice" : " voices");
    if (id == ampAttack || id == b3AmpAttack)   return formatMs (ampAttackToMs (v));
    if (id == ampDecay || id == b3AmpDecay)     return ampDecayHolds (v) ? String ("Hold") : formatMs (ampDecayToMs (v));
    if (id == ampRelease || id == b3AmpRelease) return formatMs (ampReleaseToMs (v));
    if (id == echoFeedback)              return String (roundToInt (echoFeedbackGain (v) * 100.0f)) + " %";
    if (id == echoTime)                  return formatMs (echoFreeMs (v)) + " / " + echoDivisions()[(size_t) divisionIndex (v, (int) echoDivisions().size())].name;
    if (id == tubeEdgeLow || id == tubeXover1 || id == tubeXover2 || id == tubeEdgeHigh) return formatHz (v);
    if (endsWith (id, "Gain"))           return signedText (v, " dB");
    if (endsWith (id, "Depth"))          return signedText (v, " %");
    if (endsWith (id, "Phase"))          return formatNumber (v) + " deg";
    if (endsWith (id, "Rate") && id.startsWith ("lfo"))
        return formatNumber (lfoFreeHz (v)) + " Hz / " + lfoDivisions()[(size_t) divisionIndex (v, (int) lfoDivisions().size())].name;
    return formatNumber (v) + " %";
}

String helpForParameter (const String& id)
{
    using namespace ParamIDs;
    static const std::map<String, String> help =
    {
        { tune,        "Master tuning in semitones." },
        { volume,      "Master output level." },
        { echoOn,      "Delay on / off. Switching off lets the repeats ring out." },
        { tubeOn,      "Three-band tube distortion on / off (click-free)." },
        { wave,        "Oscillator wave: saw, or the square shaped from the saw." },
        { retro,       "Vintage drift: slow pitch wander, per-note variation, softer top end." },
        { cutoffLow,   "Floor the filter envelope falls back to. At or above Cutoff it behaves classically." },
        { cutoff,      "Filter cutoff: where the resonant peak sits." },
        { resonance,   "Filter resonance. The diode model squelches but never whistles." },
        { transistor,  "Switch the filter from the diode ladder to a transistor ladder." },
        { engine,      "ACID or 303: the synth this page shows. The sequencer plays the one you choose." },
        { oscAOctave,  "Osc 1 octave: up or down by whole octaves." },
        { oscBOctave,  "Osc 2 octave: up or down by whole octaves." },
        { b3Wave,      "303 waveform: saw, or the square shaped from it." },
        { b3Tuning,    "303 tuning in semitones." },
        { b3Octave,    "303 octave: up or down by whole octaves." },
        { b3Cutoff,    "303 cutoff (about 300 Hz - 2.7 kHz before the envelope)." },
        { b3Resonance, "303 resonance: the squelch. More resonance also gives more accent sweep." },
        { b3EnvMod,    "303 envelope amount. Like the hardware, it also lowers the base cutoff." },
        { b3Decay,     "303 filter decay for normal notes. Accented notes always use the shortest decay." },
        { b3Accent,    "303 accent: louder, brighter accented notes; runs of accents build the cutoff up." },
        { b3Volume,    "303 volume." },
        { b3Model,     "VCO model: Classic (a little analog wander), Stable (clean), Drift (more wander)." },
        { b3AccurateGate, "Softer, slightly longer gate edges, like the hardware's VCA." },
        { b3VcaNoise,  "The faint hiss of the hardware's VCA." },
        { b3Vibrato,   "A gentle 5.5 Hz vibrato." },
        { b3PhaseReset, "The oscillator restarts on every new note." },
        { b3SlowSlide, "An aged slide capacitor: twice the glide time." },
        { b3WeakAccentCap, "An aged accent capacitor: accents build up further." },
        { b3InfSustain, "The VCA holds while the gate is open instead of decaying." },
        { b3SoftAccent, "A gentler accent." },
        { b3AmpAttack,  "303 amp attack." },
        { b3AmpDecay,   "303 amp decay while the gate is held (fully right = hold)." },
        { b3AmpRelease, "303 amp release after the gate closes." },
        { b3AmpSync,    "Sync: the 303 amp Attack, Decay and Release in note values from 1/64 to 1/1, following the tempo." },
        { b3VelLink,    "Velocity: the sequencer's Velocity lane (or the keys you play) sets each 303 note's level and filter." },
        { distPageOn,  "Dist: the whole Distortion page on / off." },
        { distOn,      "Saturate / Foldback distortion on / off." },
        { distMode,    "Saturate: hard asymmetric clipping. Foldback: the wave folds back on itself. Overdrive: a pedal-style hard clip. "
                       "Fuzz: square-wave fuzz. Scream: a resonant mid boost into the clipper. Rectify: rectified, an octave of grit on top." },
        { distDrive,   "How hard the distortion is driven." },
        { distAmount,  "Distort: how much of the distorted signal you hear." },
        { filterMode,  "Filter type: low-pass (classic), band-pass or high-pass, all from the same ladder." },
        { oscALock,    "Phase lock: Osc 1 (and its unison voices) starts every note at the same point of its wave." },
        { oscBLock,    "Phase lock: Osc 2 starts every note at the same point of its wave." },
        { oscSync,     "Hard Sync: Osc 1 is locked to the note and swept up by the Sync knob - the classic tearing sync sound." },
        { oscSyncPitch, "Sync: how far Osc 1 sweeps above the note while Hard Sync is on (semitones). Turn it for the sync sweep." },
        { fmAmount,    "FM: how far one oscillator modulates the other's pitch." },
        { fmDirection, "FM: from > into. 1 > 2: Osc 1 drives Osc 2 (the FM knob brings the modulated Osc 2 in). 2 > 1: Osc 2 drives Osc 1." },
        { rndGlide,    "Randomise: the chance a playing step glides (slides) into the next." },
        { rndLockGlide, "Keep the current glides when randomising." },
        { filterVelLink, "Vel Link: velocity moves the cutoff. The sequencer's Velocity lane when it plays, the keys otherwise." },
        { filterVelAmount, "How far low velocities close the cutoff (100 % = up to 4 octaves at the lowest velocity; 127 = no change)." },
        { bass,        "Low shelf after the filter." },
        { treble,      "High shelf after the filter." },
        { drive,       "Asymmetric overdrive after the VCA." },
        { envMod,      "How far the filter envelope opens the cutoff (up to 5 octaves)." },
        { decay,       "Filter envelope decay for normal notes." },
        { accent,      "Accent strength: louder notes and a filter kick." },
        { envAttack,   "Soft attack of the filter envelope." },
        { accDecay,    "Filter envelope decay for accented notes." },
        { accSweep,    "Accent sweep: consecutive accents build the cutoff up higher and higher." },
        { slideTime,   "Glide time: how long a Glide step takes to slide to the next step's pitch." },
        { arpOn,       "Arpeggiator / sequencer on. Off: incoming MIDI plays the synth directly." },
        { arpMode,     "Arp mode. Step and Chord play the step sequencer." },
        { arpRate,     "Step length, synced to the tempo." },
        { arpRateType, "Straight, triplet or dotted note values." },
        { arpGate,     "Note length as 1-200 % of a step. Above 100 % notes overlap." },
        { arpSwing,    "Moves every second step later (+) or earlier (-) by up to half a step." },
        { arpOctave,   "Each full cycle repeats one octave higher, up to this many octaves." },
        { velMode,     "Key, Hold, Step, Step+Key or Step+Hold velocity." },
        { bpm,         "Internal tempo, used when the host gives none." },
        { autoPlay,    "Auto Play: the sequencer plays whenever the host plays, from the Key's root. Keys you play take over." },
        { midiOut,     "Send the sequencer's notes to the plugin's MIDI output." },
        { seqLength,   "Number of steps: 16 or 32." },
        { seqEnd,      "End: No = play every step; a step number loops back to step 1 after that step." },
        { scaleKey,    "Key (root) of the scale." },
        { scale,       "Scale used by Scale Lock." },
        { scaleLock,   "Scale Lock: the Transpose numbers count notes of the scale, so everything stays in key." },
        { rndDensity,  "Randomise: the chance a step plays." },
        { rndRange,    "Randomise: the transpose range." },
        { rndVelMin,   "Randomise: lowest velocity." },
        { rndVelMax,   "Randomise: highest velocity." },
        { rndRatchet,  "Randomise: the chance a playing step gets 2-4 ratchet hits." },
        { rndAccent,   "Randomise: the chance a playing step is accented." },
        { rndLockPitch, "Keep the current transposes when randomising." },
        { rndLockVel,  "Keep the current velocities when randomising." },
        { rndLockRest, "Keep the Volume lane (and so the rests) when randomising." },
        { rndLockRatchet, "Keep the current ratchets when randomising." },
        { rndLockAccent, "Keep the current accents when randomising." },
        { rndFileNotes, "File Notes: Randomise picks pitches only from the last imported file's notes." },
        { exportBars,  "Length of the exported MIDI clip." },
        { exportRoot,  "Export root note (Auto = the key you last played)." },
        { exportChord, "Chord used for export in note modes when no keys have been played." },
        { oscBLevel,   "Level of the second oscillator layer." },
        { oscBTune,    "Second oscillator tuning in semitones." },
        { oscBDetune,  "Second oscillator fine detune in cents." },
        { unisonVoices, "Number of stacked unison oscillators (still one mono voice)." },
        { unisonSpread, "Unison detune and stereo spread." },
        { unisonFree,  "Free: unison phases run freely. Off: they reset on every note." },
        { ampAttack,   "VCA attack." },
        { ampDecay,    "VCA decay while the gate is held (fully right = hold)." },
        { ampRelease,  "VCA release after the gate closes." },
        { ampSync,     "Sync: the amp Attack, Decay and Release in note values from 1/64 to 1/1, following the tempo." },
        { distSaturate,  "Saturate on / off: hard, lopsided clipping. Any number of modes can be on; they run one after another." },
        { distFoldback,  "Foldback on / off: the wave folds back on itself." },
        { distOverdrive, "Overdrive on / off: a pedal-style hard clip." },
        { distFuzz,      "Fuzz on / off: square-edged fuzz that sputters as notes fade." },
        { distScream,    "Scream on / off: a resonant mid peak driven into a hard clip." },
        { distRectify,   "Rectify on / off: harsh and metallic, an octave of grit on top." },
        { echoSync,    "Echo time in note values (on) or milliseconds (off)." },
        { echoTime,    "Echo time." },
        { echoMix,     "Echo dry/wet (equal power)." },
        { echoFeedback, "Echo repeats." },
        { echoLowCut,  "Low cut inside the echo loop: every repeat gets thinner." },
        { echoHighCut, "High cut inside the echo loop: every repeat gets darker." },
        { echoWidth,   "0 = mono echo, 100 = full ping-pong." },
        { tubeResonator, "Resonant emphasis in each band before the tubes." },
        { tubeCharacter, "Tube curve: symmetric (odd harmonics) to asymmetric (even)." },
        { tubeInput,   "Level into the tubes." },
        { tubeOutput,  "Level out of the tubes." },
        { tubeMix,     "Dry/wet of the tube distortion." },
        { tubeDrive,   "Tube drive: how hard the signal hits the tube." },
        { tubeEdgeLow, "Low edge: below it the signal passes clean." },
        { tubeXover1,  "Crossover between the low and mid bands." },
        { tubeXover2,  "Crossover between the mid and high bands." },
        { tubeEdgeHigh, "High edge: above it the signal passes clean." },
    };

    if (auto it = help.find (id); it != help.end())
        return it->second;

    if (id.startsWith ("lfo"))
    {
        if (id.endsWith ("Rate"))   return "LFO speed (note values when Sync is on).";
        if (id.endsWith ("Depth"))  return "LFO amount (negative = inverted).";
        if (id.endsWith ("Phase"))  return "LFO start phase.";
        if (id.endsWith ("Shape"))  return "LFO waveform.";
        if (id.endsWith ("Sync"))   return "Sync the LFO rate to the tempo.";
        if (id.endsWith ("Mirror")) return "Mirror: unipolar 0..1 instead of -1..+1.";
        if (id.endsWith ("Retrig")) return "Free running, restart on every note, or locked to the bar.";
        if (id.endsWith ("Dest"))   return "What the LFO modulates.";
    }
    if (id.endsWith ("Drive")) return "Tube drive of this band.";
    if (id.endsWith ("Gain"))  return "Level of this band after its tube.";
    if (id.endsWith ("Solo"))  return "Listen to this band alone.";
    return {};
}

//==============================================================================
AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto number = [&layout] (const String& id, const String& name, float lo, float hi, float def)
    {
        auto attributes = AudioParameterFloatAttributes()
            .withStringFromValueFunction ([id] (float v, int) { return describeParameter (id, v); })
            .withValueFromStringFunction ([id] (const String& s) { return parseParameter (id, s); });
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name,
                                                            NormalisableRange<float> (lo, hi, 1.0f), def, attributes));
    };
    auto knob = [&number] (const String& id, const String& name, float def) { number (id, name, 0.0f, 100.0f, def); };
    // in Hz, exact to 1 Hz; the host's automation lane (and the knob) run on a log scale
    auto hzKnob = [&layout] (const String& id, const String& name, float def)
    {
        const auto r = hzKnobRange (id);
        const float lo = r.getStart(), hi = r.getEnd();
        NormalisableRange<float> range (lo, hi,
            [] (float a, float b, float p) { return a * std::pow (b / a, jlimit (0.0f, 1.0f, p)); },
            [] (float a, float b, float v) { return jlimit (0.0f, 1.0f, std::log (jlimit (a, b, v) / a) / std::log (b / a)); },
            [] (float a, float b, float v) { return jlimit (a, b, std::round (v)); });
        auto attributes = AudioParameterFloatAttributes()
            .withStringFromValueFunction ([id] (float v, int) { return describeParameter (id, v); })
            .withValueFromStringFunction ([id] (const String& s) { return parseParameter (id, s); });
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, range, def, attributes));
    };

    auto frequency = [&layout] (const String& id, const String& name, float def)
    {
        NormalisableRange<float> range (20.0f, 20000.0f, 1.0f);
        range.setSkewForCentre (630.0f);
        auto attributes = AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) { return formatHz (v); })
            .withValueFromStringFunction ([] (const String& s) { return s.getFloatValue() * (s.containsIgnoreCase ("k") ? 1000.0f : 1.0f); });
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, range, def, attributes));
    };

    auto choice = [&layout] (const String& id, const String& name, const StringArray& items, int def)
    {
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, items, def));
    };
    auto toggle = [&layout] (const String& id, const String& name, bool def)
    {
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def));
    };

    using namespace ParamIDs;

    // Header
    number (tune,   "Tune",   -12.0f, 12.0f, 0.0f);
    number (volume, "Volume", -36.0f, 6.0f, 0.0f);
    toggle (echoOn, "Delay On", true);
    toggle (tubeOn, "Tube On", true);

    // Synth
    choice (wave, "Wave", waveNames(), 0);
    toggle (retro, "Retro", false);
    hzKnob (cutoffLow, "Cutoff Low", 160.0f);
    hzKnob (cutoff,    "Cutoff",     450.0f);
    knob (resonance, "Resonance",  65.0f);
    toggle (transistor, "Transistor", false);
    choice (engine, "Synth", engineNames(), 0);
    number (oscAOctave, "Osc 1 Octave", -2.0f, 2.0f, 0.0f);
    number (oscBOctave, "Osc 2 Octave", -2.0f, 2.0f, 0.0f);

    // 303 page
    choice (b3Wave, "303 Waveform", b3WaveNames(), 0);
    number (b3Tuning, "303 Tuning", -12.0f, 12.0f, 0.0f);
    number (b3Octave, "303 Octave", -2.0f, 2.0f, 0.0f);
    knob (b3Cutoff,    "303 Cutoff",    45.0f);
    knob (b3Resonance, "303 Resonance", 70.0f);
    knob (b3EnvMod,    "303 Env Mod",   55.0f);
    knob (b3Decay,     "303 Decay",     40.0f);
    knob (b3Accent,    "303 Accent",    65.0f);
    knob (b3Volume,    "303 Volume",    70.0f);
    choice (b3Model, "303 VCO Model", b3ModelNames(), 0);
    toggle (b3AccurateGate,  "303 Accurate Gate", false);
    toggle (b3VcaNoise,      "303 VCA Noise", true);
    toggle (b3Vibrato,       "303 Vibrato", false);
    toggle (b3PhaseReset,    "303 Phase Reset", false);
    toggle (b3SlowSlide,     "303 Slow Slide", false);
    toggle (b3WeakAccentCap, "303 Weak Accent Cap", false);
    toggle (b3InfSustain,    "303 Infinite Sustain", false);
    toggle (b3SoftAccent,    "303 Soft Accent", false);
    knob (b3AmpAttack,  "303 Amp Attack",  0.0f);     // 1 ms
    knob (b3AmpDecay,   "303 Amp Decay",   84.0f);    // ~3.2 s, the hardware's slow VCA decay
    knob (b3AmpRelease, "303 Amp Release", 0.0f);     // 10 ms
    toggle (b3AmpSync,  "303 Amp Sync", false);
    toggle (b3VelLink,       "303 Velocity", false);

    // Distortion page
    toggle (distPageOn, "Distortion On", true);
    toggle (distOn, "Saturate/Foldback On", false);
    toggle (distSaturate,  "Dist Saturate", true);
    toggle (distFoldback,  "Dist Foldback", false);
    toggle (distOverdrive, "Dist Overdrive", false);
    toggle (distFuzz,      "Dist Fuzz", false);
    toggle (distScream,    "Dist Scream", false);
    toggle (distRectify,   "Dist Rectify", false);
    choice (distMode, "Distortion Mode", distModeNames(), 0);
    knob (distDrive,  "Distortion Drive", 50.0f);
    knob (distAmount, "Distortion Amount", 70.0f);

    choice (filterMode, "Filter Type", filterModeNames(), 0);
    toggle (oscALock, "Osc 1 Phase Lock", false);
    toggle (oscBLock, "Osc 2 Phase Lock", false);
    toggle (oscSync, "Osc Hard Sync", false);
    number (oscSyncPitch, "Sync Pitch", 0.0f, 48.0f, 12.0f);
    knob (fmAmount, "FM Amount", 0.0f);
    choice (fmDirection, "FM Direction", fmDirectionNames(), 0);
    toggle (filterVelLink, "Filter Velocity Link", false);
    knob (filterVelAmount, "Filter Velocity Amount", 50.0f);
    number (bass,   "Bass",   -12.0f, 12.0f, 0.0f);
    number (treble, "Treble", -12.0f, 12.0f, 0.0f);
    knob (drive,     "Drive",      15.0f);
    knob (envMod,    "Env Mod",    55.0f);
    knob (decay,     "Decay",      45.0f);
    knob (accent,    "Accent",     60.0f);
    knob (envAttack, "Env Attack", 50.0f);    // 3 ms
    knob (accDecay,  "Acc Decay",  35.0f);
    knob (accSweep,  "Acc Sweep",  40.0f);

    // Sequencer: the ArpSampler arpeggiator + step sequencer (Glide = slideTime)
    number (slideTime, "Glide", 10.0f, 500.0f, 60.0f);
    toggle (arpOn, "Arp On", true);
    choice (arpMode, "Arp Mode", as::getArpModeNames(), (int) as::ArpMode::Step);
    choice (arpRate, "Arp Rate", as::getRateNames(), 3);
    choice (arpRateType, "Arp Rate Type", as::getRateTypeNames(), 0);
    number (arpGate, "Arp Gate", 1.0f, 200.0f, 50.0f);
    number (arpSwing, "Arp Swing", -100.0f, 100.0f, 0.0f);
    number (arpOctave, "Arp Octave", 1.0f, 4.0f, 1.0f);
    choice (velMode, "Velocity Mode", as::getVelModeNames(), 0);
    number (bpm, "Internal BPM", 20.0f, 300.0f, as::kDefaultBpm);
    toggle (autoPlay, "Auto Play", true);
    toggle (midiOut, "MIDI Out", false);
    choice (seqLength, "Seq Length", as::getSeqLengthNames(), 0);
    choice (seqEnd, "Seq End", as::getSeqEndNames(), 0);
    choice (scaleKey, "Scale Key", as::getKeyNames(), 0);
    choice (scale, "Scale", as::getScaleNames(), 0);
    toggle (scaleLock, "Scale Lock", false);

    // Randomiser and export settings: saved with the project, not automatable
    auto setting = [&layout] (const String& id, const String& name, float lo, float hi, float def)
    {
        auto attributes = AudioParameterFloatAttributes()
            .withStringFromValueFunction ([id] (float v, int) { return describeParameter (id, v); })
            .withAutomatable (false);
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (lo, hi, 1.0f), def, attributes));
    };
    auto settingToggle = [&layout] (const String& id, const String& name, bool def)
    {
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def, AudioParameterBoolAttributes().withAutomatable (false)));
    };
    auto settingChoice = [&layout] (const String& id, const String& name, const StringArray& items, int def)
    {
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, items, def, AudioParameterChoiceAttributes().withAutomatable (false)));
    };
    setting (rndDensity, "Rnd Density", 0.0f, 100.0f, 75.0f);
    setting (rndRange, "Rnd Pitch Range", 0.0f, 24.0f, 12.0f);
    setting (rndVelMin, "Rnd Vel Min", 1.0f, 127.0f, 60.0f);
    setting (rndVelMax, "Rnd Vel Max", 1.0f, 127.0f, 127.0f);
    setting (rndRatchet, "Rnd Ratchet Prob", 0.0f, 100.0f, 10.0f);
    setting (rndAccent, "Rnd Accent Prob", 0.0f, 100.0f, 25.0f);
    setting (rndGlide, "Rnd Glide Prob", 0.0f, 100.0f, 20.0f);
    settingToggle (rndLockGlide, "Rnd Lock Glides", false);
    settingToggle (rndLockPitch, "Rnd Lock Pitch", false);
    settingToggle (rndLockVel, "Rnd Lock Velocity", false);
    settingToggle (rndLockRest, "Rnd Lock Rests", false);
    settingToggle (rndLockRatchet, "Rnd Lock Ratchets", false);
    settingToggle (rndLockAccent, "Rnd Lock Accents", false);
    settingToggle (rndFileNotes, "Rnd File Notes", false);
    settingChoice (exportBars, "Export Length", as::getExportBarNames(), 1);
    setting (exportRoot, "Export Root", -1.0f, 127.0f, -1.0f);
    settingChoice (exportChord, "Export Chord", as::getExportChordNames(), 0);

    // Layers
    knob (oscBLevel, "Osc 2 Level", 0.0f);
    number (oscBTune,   "Osc 2 Tune",   -24.0f, 24.0f, 0.0f);
    number (oscBDetune, "Osc 2 Detune", -50.0f, 50.0f, 8.0f);
    number (unisonVoices, "Unison Voices", 1.0f, 8.0f, 1.0f);
    knob (unisonSpread, "Unison Spread", 25.0f);
    toggle (unisonFree, "Unison Free", true);
    knob (ampAttack,  "Amp Attack",  0.0f);    // 1 ms
    knob (ampDecay,   "Amp Decay",   100.0f);  // hold
    knob (ampRelease, "Amp Release", 0.0f);    // 10 ms
    toggle (ampSync, "Amp Sync", false);
    toggle (echoSync, "Delay Sync", true);
    knob (echoTime,     "Delay Time",     knobForDivision (8, (int) echoDivisions().size()));   // 1/8
    knob (echoMix,      "Delay Mix",      15.0f);
    knob (echoFeedback, "Delay Feedback", 45.0f);
    hzKnob (echoLowCut,  "Delay Low Cut",  83.0f);
    hzKnob (echoHighCut, "Delay High Cut", 4340.0f);
    knob (echoWidth,    "Delay Width",    80.0f);

    for (int i = 0; i < 2; ++i)
    {
        const String n = "LFO " + String (i + 1) + " ";
        const int defDiv = i == 0 ? 7 : 10;   // 1/4, 1/8
        knob (lfo (i, "Rate"), n + "Rate", knobForDivision (defDiv, (int) lfoDivisions().size()));
        number (lfo (i, "Depth"), n + "Depth", -100.0f, 100.0f, 0.0f);
        number (lfo (i, "Phase"), n + "Phase", 0.0f, 360.0f, 0.0f);
        choice (lfo (i, "Shape"), n + "Shape", lfoShapeNames(), 1);
        toggle (lfo (i, "Sync"), n + "Sync", true);
        toggle (lfo (i, "Mirror"), n + "Mirror", false);
        choice (lfo (i, "Retrig"), n + "Retrig", lfoRetrigNames(), 0);
        choice (lfo (i, "Dest"), n + "Destination", lfoDestNames(), DestCutoff);
    }

    // Tube
    frequency (tubeEdgeLow,  "Tube Low Edge",    62.0f);
    frequency (tubeXover1,   "Tube Crossover 1", 748.0f);
    frequency (tubeXover2,   "Tube Crossover 2", 2100.0f);
    frequency (tubeEdgeHigh, "Tube High Edge",   11000.0f);
    const char* bandNames[] = { "Low", "Mid", "High" };
    for (int b = 0; b < 3; ++b)
    {
        const String n = String ("Tube ") + bandNames[b] + " ";
        knob (band (b, "Drive"), n + "Drive", 30.0f);
        number (band (b, "Gain"), n + "Gain", -15.0f, 15.0f, 0.0f);
        toggle (band (b, "Solo"), n + "Solo", false);
    }
    knob (tubeResonator, "Tube Resonator", 20.0f);
    knob (tubeCharacter, "Tube Character", 30.0f);
    number (tubeInput,  "Tube Input",  -24.0f, 24.0f, 0.0f);
    number (tubeOutput, "Tube Output", -24.0f, 24.0f, 0.0f);
    knob (tubeMix, "Tube Mix", 100.0f);
    knob (tubeDrive,  "Tube Drive", 30.0f);

    return layout;
}
} // namespace acid
