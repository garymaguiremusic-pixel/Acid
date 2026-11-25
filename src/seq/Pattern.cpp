#include "Pattern.h"
#include <algorithm>

namespace acid
{
std::string patternToString (const Pattern& p)
{
    static const char* hex = "0123456789abcdef";
    std::string s = std::to_string (p.length) + ":";
    for (const auto& st : p.steps)
    {
        s += hex[std::clamp ((int) st.note, 0, 12)];
        s += (char) ('0' + std::clamp ((int) st.octave + 2, 0, 4));
        s += st.accent ? '1' : '0';
        s += st.slide ? '1' : '0';
        s += (char) ('0' + std::clamp ((int) st.ratchet, 0, kMaxRatchet));
        s += st.gate == Gate::Rest ? 'R' : (st.gate == Gate::Tie ? 'T' : 'N');
    }
    return s;
}

bool patternFromString (const std::string& s, Pattern& out)
{
    const auto colon = s.find (':');
    if (colon == std::string::npos)
        return false;

    Pattern p;
    try { p.length = std::clamp (std::stoi (s.substr (0, colon)), 1, kMaxSteps); }
    catch (...) { return false; }

    auto hexValue = [] (char c) { return c >= '0' && c <= '9' ? c - '0' : (c >= 'a' && c <= 'f' ? c - 'a' + 10 : 0); };

    size_t pos = colon + 1;
    for (auto& st : p.steps)
    {
        if (pos + 6 > s.size())
            break;
        st.note    = (uint8_t) std::clamp (hexValue (s[pos]), 0, 12);
        st.octave  = (int8_t) std::clamp (s[pos + 1] - '0' - 2, -2, 2);
        st.accent  = s[pos + 2] == '1';
        st.slide   = s[pos + 3] == '1';
        st.ratchet = (uint8_t) std::clamp (s[pos + 4] - '0', 0, kMaxRatchet);
        st.gate    = s[pos + 5] == 'R' ? Gate::Rest : (s[pos + 5] == 'T' ? Gate::Tie : Gate::Note);
        pos += 6;
    }
    out = p;
    return true;
}

int stepMidiNote (const Step& st, int transpose) noexcept
{
    return std::clamp (36 + (int) st.note + 12 * (int) st.octave + transpose, 0, 127);
}

Pattern makeDefaultPattern()
{
    Pattern p;
    p.length = 16;
    return p;
}
} // namespace acid
