#include "FactoryPresets.h"
#include "../arp/ArpParameters.h"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace acid
{
namespace
{
    // One step of the original Acid sequencer.
    struct AcidStep
    {
        int note = 0, octave = 0, ratchet = 0;
        bool accent = false, slide = false;
        enum Kind { Note, Rest, Tie } kind = Note;
    };

    ConvertedPattern convert (const std::vector<AcidStep>& steps)
    {
        ConvertedPattern out;
        const int n = std::min ((int) steps.size(), (int) as::kMaxSteps);
        out.length = std::max (1, n);

        for (int i = 0; i < n; ++i)
        {
            const auto& a = steps[(size_t) i];
            auto& s = out.pattern.steps[(size_t) i];
            s.velocity = 100;
            s.ratchet = std::clamp (a.ratchet + 1, 1, as::arp::kMaxRatchet);
            s.accent = a.accent;

            if (a.kind == AcidStep::Rest)
            {
                s.volume = 0;
                continue;
            }
            if (a.kind == AcidStep::Tie)
            {
                // holds the previous note: the previous step ties into this one at the same pitch
                if (i == 0 || out.pattern.steps[(size_t) i - 1].isRest())
                {
                    s.volume = 0;
                    continue;
                }
                out.pattern.steps[(size_t) i - 1].tie = true;
                s.transpose = out.pattern.steps[(size_t) i - 1].transpose;
                s.ratchet = 1;
            }
            else
            {
                s.transpose = std::clamp (a.note + 12 * a.octave, -24, 24);
            }
            s.volume = 127;
            s.glide = a.slide;    // a slide glides into the next step: the sequencer's Glide lane
        }

        // a slide into a rest doesn't glide anywhere
        for (int i = 0; i < n; ++i)
            if (out.pattern.steps[(size_t) i].glide && (i + 1 >= n || out.pattern.steps[(size_t) i + 1].isRest()))
                out.pattern.steps[(size_t) i].glide = false;
        return out;
    }
}

ConvertedPattern patternFromNotation (const char* text)
{
    std::vector<AcidStep> steps;
    std::istringstream in (text);
    std::string tok;

    while (in >> tok && (int) steps.size() < as::kMaxSteps)
    {
        AcidStep st;
        size_t i = 0;
        const char c = (char) std::toupper ((unsigned char) tok[0]);

        if (c == '-')      { st.kind = AcidStep::Rest; i = 1; }
        else if (c == '_') { st.kind = AcidStep::Tie;  i = 1; }
        else
        {
            static const int semis[] = { 9, 11, 0, 2, 4, 5, 7 };   // A B C D E F G
            int n = (c >= 'A' && c <= 'G') ? semis[c - 'A'] : 0;
            i = 1;
            if (i < tok.size() && tok[i] == '#') { ++n; ++i; }
            if (i < tok.size() && tok[i] == '\'') { n = 12; ++i; }
            st.note = std::clamp (n, 0, 12);
        }

        for (; i < tok.size(); ++i)
        {
            switch (tok[i])
            {
                case '^': st.octave = std::min (2, st.octave + 1); break;
                case 'v': st.octave = std::max (-2, st.octave - 1); break;
                case '!': st.accent = true; break;
                case '~': st.slide = true; break;
                case 'x':
                    if (i + 1 < tok.size() && std::isdigit ((unsigned char) tok[i + 1]))
                        st.ratchet = std::clamp (tok[++i] - '1', 0, 3);
                    break;
                default: break;
            }
        }
        steps.push_back (st);
    }
    return convert (steps);
}

ConvertedPattern patternFromLegacyString (const std::string& s)
{
    const auto colon = s.find (':');
    if (colon == std::string::npos)
        return {};

    int length = 16;
    try { length = std::clamp (std::stoi (s.substr (0, colon)), 1, 64); }
    catch (...) { return {}; }

    auto hexValue = [] (char c) { return c >= '0' && c <= '9' ? c - '0' : (c >= 'a' && c <= 'f' ? c - 'a' + 10 : 0); };
    std::vector<AcidStep> steps;
    for (size_t pos = colon + 1; pos + 6 <= s.size() && (int) steps.size() < std::min (length, (int) as::kMaxSteps); pos += 6)
    {
        AcidStep st;
        st.note    = std::clamp (hexValue (s[pos]), 0, 12);
        st.octave  = std::clamp (s[pos + 1] - '0' - 2, -2, 2);
        st.accent  = s[pos + 2] == '1';
        st.slide   = s[pos + 3] == '1';
        st.ratchet = std::clamp (s[pos + 4] - '0', 0, 3);
        st.kind    = s[pos + 5] == 'R' ? AcidStep::Rest : (s[pos + 5] == 'T' ? AcidStep::Tie : AcidStep::Note);
        steps.push_back (st);
    }
    return convert (steps);
}

namespace
{
    // Shared patterns (all original)
    const char* kRoller   = "A~ A A^! A G A~ C' A^ A! - A A^~ G A D^! A";
    const char* kRoller2  = "A A^! A~ A - A^ G~ A! C'^ A A~ D^! A - A^~ E";
    const char* kStabby   = "C! - C^ C - D#~ F C! - C C^! - G~ A# C' -";
    const char* kWalker   = "E~ E E^ G E! B E^~ D E E! G~ A E^ - D E";
    const char* kPhrygian = "D D^! D# D~ D A# D^ D! F~ D# D - C'v D^~ D D!";
    const char* kDeep     = "F - - F~ G# - F! - C - F^ - D#~ F - -";
    const char* kHard     = "A! A^ A A^ A! A^~ C' A A! A^ A G! A^ A A^~ D^";
    const char* kSquare   = "G~ G^ A# G! - G~ D G^! F G - A#~ C'^ G! F~ D";
    const char* kRatchet  = "C!x2 - C^ D#~ F Cx3 - G~ C! C^x2 - A# C'!x4 - G~ C";
    const char* kOctaves  = "Cv C C^ C^^! Cv~ C C^~ C^^ G#v G# G#^! - A#v A# A#^~ C'";
    const char* kSwingy   = "A A^! - A~ G - A! C'^ - A~ A G~ E^ - A! -";
    const char* kNight    = "D~ D _ F A! _ D^ - C~ C _ D# G! _ C^ -";
    const char* kMinimal  = "A - - A! - - A - A^ - - A! - - D -";
    const char* kSirens   = "E^~ F^~ E^ - E^~ G^! - E^ B~ C'^ B - E^~ D^ E^! -";
    const char* kLong     = "A A^! A~ A G A C'^~ A A! A A^~ G A D^ A "
                            "A A^! A~ A G A C'^~ A F! F F^~ E F A^ G~";
}

const std::vector<FactoryPreset>& factoryPresets()
{
    using P = std::vector<std::pair<std::string, float>>;
    static const std::vector<FactoryPreset> list =
    {
        { "Squelch Classic", "CLASSIC",
          P { { "cutoff", 40 }, { "resonance", 78 }, { "envMod", 62 }, { "decay", 45 }, { "accent", 70 }, { "accSweep", 50 } },
          kRoller },
        { "Rubber Bassline", "CLASSIC",
          P { { "wave", 1 }, { "cutoff", 34 }, { "resonance", 55 }, { "envMod", 42 }, { "decay", 32 }, { "arpGate", 60 } },
          kSquare },
        { "Acid Rain", "CLASSIC",
          P { { "cutoff", 30 }, { "resonance", 88 }, { "envMod", 78 }, { "accSweep", 72 }, { "accent", 80 },
              { "lfo1Rate", 19 }, { "lfo1Depth", 18 } },
          kRoller2 },
        { "Dub Chamber", "DUB",
          P { { "cutoff", 30 }, { "resonance", 62 }, { "envMod", 45 }, { "echoMix", 34 }, { "echoFeedback", 72 },
              { "echoTime", 63 }, { "echoWidth", 100 }, { "echoLowCut", 42 }, { "echoHighCut", 30 } },
          kDeep },
        { "Siren Line", "DUB",
          P { { "cutoff", 48 }, { "resonance", 70 }, { "lfo2Dest", 1 }, { "lfo2Depth", 4 }, { "lfo2Rate", 87 },
              { "echoMix", 28 }, { "echoFeedback", 60 } },
          kSirens },
        { "Hard Floor", "HARD",
          P { { "cutoff", 38 }, { "resonance", 82 }, { "envMod", 70 }, { "drive", 55 }, { "accent", 85 },
              { "tubeLowDrive", 70 }, { "tubeMidDrive", 75 }, { "tubeHighDrive", 55 }, { "tubeOutput", -5 }, { "volume", -4 } },
          kHard },
        { "Distorted Ravelines", "HARD",
          P { { "cutoff", 42 }, { "resonance", 80 }, { "drive", 60 }, { "tubeResonator", 60 }, { "tubeCharacter", 70 },
              { "tubeMidDrive", 85 }, { "tubeMidGain", 4 }, { "tubeOutput", -6 }, { "volume", -8 } },
          kHard },
        { "Deep Minimal", "DEEP",
          P { { "cutoff", 26 }, { "resonance", 50 }, { "envMod", 40 }, { "decay", 25 }, { "arpGate", 35 }, { "echoMix", 20 } },
          kMinimal },
        { "Square Funk", "CLASSIC",
          P { { "wave", 1 }, { "cutoff", 44 }, { "resonance", 60 }, { "slideTime", 80 }, { "envMod", 50 } },
          kSquare },
        { "Unison Monster", "ACID",
          P { { "unisonVoices", 6 }, { "unisonSpread", 50 }, { "oscBLevel", 40 }, { "oscBTune", -12 },
              { "cutoff", 36 }, { "resonance", 70 }, { "tubeLowDrive", 50 }, { "volume", -4 } },
          kRoller },
        { "Transistor Scream", "ACID",
          P { { "transistor", 1 }, { "cutoff", 42 }, { "resonance", 80 }, { "envMod", 60 }, { "drive", 35 }, { "volume", -12 } },
          kRoller2 },
        { "Retro Box", "CLASSIC",
          P { { "retro", 1 }, { "cutoff", 41 }, { "resonance", 72 }, { "drive", 5 }, { "echoOn", 0 } },
          kRoller },
        { "Ping-Pong Acid", "DUB",
          P { { "cutoff", 40 }, { "resonance", 74 }, { "echoWidth", 100 }, { "echoMix", 26 }, { "echoFeedback", 55 } },
          kStabby },
        { "Wobble Acid", "ACID",
          P { { "cutoff", 35 }, { "resonance", 75 }, { "lfo1Rate", 67 }, { "lfo1Depth", 35 }, { "lfo1Retrig", 2 } },
          kWalker },
        { "Tremolo Gate", "ACID",
          P { { "cutoff", 44 }, { "resonance", 66 }, { "lfo2Dest", 4 }, { "lfo2Shape", 4 }, { "lfo2Rate", 87 },
              { "lfo2Depth", 70 }, { "lfo2Retrig", 2 } },
          kLong },
        { "Slow Sweep", "DEEP",
          P { { "cutoff", 28 }, { "resonance", 76 }, { "envMod", 35 }, { "lfo1Shape", 2 }, { "lfo1Rate", 7 },
              { "lfo1Depth", 50 }, { "lfo1Retrig", 2 }, { "lfo1Mirror", 1 } },
          kLong },
        { "Ratchet Run", "HARD",
          P { { "cutoff", 40 }, { "resonance", 78 }, { "drive", 30 }, { "arpGate", 45 }, { "volume", -3 } },
          kRatchet },
        { "Octave Jumper", "ACID",
          P { { "cutoff", 39 }, { "resonance", 70 }, { "slideTime", 45 }, { "accSweep", 60 } },
          kOctaves },
        { "Swing Acid", "CLASSIC",
          P { { "cutoff", 41 }, { "resonance", 73 }, { "arpSwing", 53 }, { "envMod", 58 } },
          kSwingy },
        { "Night Drive", "DEEP",
          P { { "arpRate", 2 }, { "cutoff", 30 }, { "resonance", 68 }, { "decay", 70 }, { "accDecay", 55 },
              { "echoMix", 30 }, { "echoFeedback", 64 }, { "echoTime", 69 } },
          kNight },
    };
    return list;
}
} // namespace acid
