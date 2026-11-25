#include "FactoryPresets.h"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace acid
{
Pattern patternFromNotation (const char* text)
{
    Pattern p;
    std::istringstream in (text);
    std::string tok;
    int count = 0;

    while (in >> tok && count < kMaxSteps)
    {
        Step st;
        size_t i = 0;
        const char c = (char) std::toupper ((unsigned char) tok[0]);

        if (c == '-')      { st.gate = Gate::Rest; i = 1; }
        else if (c == '_') { st.gate = Gate::Tie;  i = 1; }
        else
        {
            static const int semis[] = { 9, 11, 0, 2, 4, 5, 7 };   // A B C D E F G
            int n = (c >= 'A' && c <= 'G') ? semis[c - 'A'] : 0;
            i = 1;
            if (i < tok.size() && tok[i] == '#') { ++n; ++i; }
            if (i < tok.size() && tok[i] == '\'') { n = 12; ++i; }
            st.note = (uint8_t) std::clamp (n, 0, 12);
        }

        for (; i < tok.size(); ++i)
        {
            switch (tok[i])
            {
                case '^': st.octave = (int8_t) std::min (2, st.octave + 1); break;
                case 'v': st.octave = (int8_t) std::max (-2, st.octave - 1); break;
                case '!': st.accent = true; break;
                case '~': st.slide = true; break;
                case 'x':
                    if (i + 1 < tok.size() && std::isdigit ((unsigned char) tok[i + 1]))
                        st.ratchet = (uint8_t) std::clamp (tok[++i] - '1', 0, kMaxRatchet);
                    break;
                default: break;
            }
        }
        p.steps[(size_t) count++] = st;
    }
    p.length = std::max (1, count);
    return p;
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
          { kRoller, kRoller2, kStabby, kWalker } },
        { "Rubber Bassline", "CLASSIC",
          P { { "wave", 1 }, { "cutoff", 34 }, { "resonance", 55 }, { "envMod", 42 }, { "decay", 32 }, { "gate", 60 } },
          { kSquare, kWalker } },
        { "Acid Rain", "CLASSIC",
          P { { "cutoff", 30 }, { "resonance", 88 }, { "envMod", 78 }, { "accSweep", 72 }, { "accent", 80 },
              { "lfo1Rate", 19 }, { "lfo1Depth", 18 } },
          { kRoller2, kRoller } },
        { "Dub Chamber", "DUB",
          P { { "cutoff", 30 }, { "resonance", 62 }, { "envMod", 45 }, { "echoMix", 34 }, { "echoFeedback", 72 },
              { "echoTime", 63 }, { "echoWidth", 100 }, { "echoLowCut", 42 }, { "echoHighCut", 30 } },
          { kDeep, kMinimal } },
        { "Siren Line", "DUB",
          P { { "cutoff", 48 }, { "resonance", 70 }, { "lfo2Dest", 1 }, { "lfo2Depth", 4 }, { "lfo2Rate", 87 },
              { "echoMix", 28 }, { "echoFeedback", 60 } },
          { kSirens } },
        { "Hard Floor", "HARD",
          P { { "cutoff", 38 }, { "resonance", 82 }, { "envMod", 70 }, { "drive", 55 }, { "accent", 85 },
              { "tubeLowDrive", 70 }, { "tubeMidDrive", 75 }, { "tubeHighDrive", 55 }, { "tubeOutput", -5 }, { "volume", -4 } },
          { kHard, kRoller } },
        { "Distorted Ravelines", "HARD",
          P { { "cutoff", 42 }, { "resonance", 80 }, { "drive", 60 }, { "tubeResonator", 60 }, { "tubeCharacter", 70 },
              { "tubeMidDrive", 85 }, { "tubeMidGain", 4 }, { "tubeOutput", -6 }, { "volume", -8 } },
          { kHard, kRatchet } },
        { "Deep Minimal", "DEEP",
          P { { "cutoff", 26 }, { "resonance", 50 }, { "envMod", 40 }, { "decay", 25 }, { "gate", 35 }, { "echoMix", 20 } },
          { kMinimal, kDeep } },
        { "Square Funk", "CLASSIC",
          P { { "wave", 1 }, { "cutoff", 44 }, { "resonance", 60 }, { "slideTime", 80 }, { "envMod", 50 } },
          { kSquare, kStabby } },
        { "Unison Monster", "ACID",
          P { { "unisonVoices", 6 }, { "unisonSpread", 50 }, { "oscBLevel", 40 }, { "oscBTune", -12 },
              { "cutoff", 36 }, { "resonance", 70 }, { "tubeLowDrive", 50 }, { "volume", -4 } },
          { kRoller, kOctaves } },
        { "Transistor Scream", "ACID",
          P { { "transistor", 1 }, { "cutoff", 42 }, { "resonance", 80 }, { "envMod", 60 }, { "drive", 35 }, { "volume", -12 } },
          { kRoller2, kHard } },
        { "Retro Box", "CLASSIC",
          P { { "retro", 1 }, { "cutoff", 41 }, { "resonance", 72 }, { "drive", 5 }, { "echoOn", 0 } },
          { kRoller, kWalker } },
        { "Ping-Pong Acid", "DUB",
          P { { "cutoff", 40 }, { "resonance", 74 }, { "echoWidth", 100 }, { "echoMix", 26 }, { "echoFeedback", 55 } },
          { kStabby, kRoller } },
        { "Wobble Acid", "ACID",
          P { { "cutoff", 35 }, { "resonance", 75 }, { "lfo1Rate", 67 }, { "lfo1Depth", 35 }, { "lfo1Retrig", 2 } },
          { kWalker, kRoller2 } },
        { "Tremolo Gate", "ACID",
          P { { "cutoff", 44 }, { "resonance", 66 }, { "lfo2Dest", 4 }, { "lfo2Shape", 4 }, { "lfo2Rate", 87 },
              { "lfo2Depth", 70 }, { "lfo2Retrig", 2 } },
          { kLong } },
        { "Slow Sweep", "DEEP",
          P { { "cutoff", 28 }, { "resonance", 76 }, { "envMod", 35 }, { "lfo1Shape", 2 }, { "lfo1Rate", 7 },
              { "lfo1Depth", 50 }, { "lfo1Retrig", 2 }, { "lfo1Mirror", 1 } },
          { kLong, kRoller } },
        { "Ratchet Run", "HARD",
          P { { "cutoff", 40 }, { "resonance", 78 }, { "drive", 30 }, { "gate", 45 }, { "volume", -3 } },
          { kRatchet, kStabby } },
        { "Octave Jumper", "ACID",
          P { { "cutoff", 39 }, { "resonance", 70 }, { "slideTime", 45 }, { "accSweep", 60 } },
          { kOctaves, kPhrygian } },
        { "Swing Acid", "CLASSIC",
          P { { "cutoff", 41 }, { "resonance", 73 }, { "swing", 40 }, { "envMod", 58 } },
          { kSwingy, kRoller } },
        { "Night Drive", "DEEP",
          P { { "seqRate", 0 }, { "cutoff", 30 }, { "resonance", 68 }, { "decay", 70 }, { "accDecay", 55 },
              { "echoMix", 30 }, { "echoFeedback", 64 }, { "echoTime", 69 } },
          { kNight, kPhrygian } },
    };
    return list;
}
} // namespace acid
