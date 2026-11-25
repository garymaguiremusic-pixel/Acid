#include "Ladder.h"

namespace acid
{
namespace
{
    // dy/dt = wc * (A y + b u). Rows below are A; b = (1, 0, 0, 0).
    constexpr float kDiodeA[4][4] = { { -2.0f,  1.0f,  0.0f,  0.0f },
                                      {  1.0f, -2.0f,  1.0f,  0.0f },
                                      {  0.0f,  1.0f, -2.0f,  1.0f },
                                      {  0.0f,  0.0f,  1.0f, -1.0f } };

    constexpr float kTransA[4][4] = { { -1.0f,  0.0f,  0.0f,  0.0f },
                                      {  1.0f, -1.0f,  0.0f,  0.0f },
                                      {  0.0f,  1.0f, -1.0f,  0.0f },
                                      {  0.0f,  0.0f,  1.0f, -1.0f } };

    // The open-loop phase reaches -180 degrees at this multiple of wc; scaling by its
    // inverse puts the resonant peak on the requested cutoff.
    constexpr float kDiodeTune = 1.0f / 1.1951f;
    constexpr float kTransTune = 1.0f;

    constexpr float kDiodeMaxFeedback = 0.94f;   // never quite a clean sine
    constexpr float kTransMaxFeedback = 1.02f;   // self-oscillates at the top
    constexpr float kFeedbackHpHz     = 110.0f;

    /** Solves the tridiagonal system M y = r (in place in r). */
    inline void solveTridiagonal (const float (&lower)[4], const float (&diag)[4], const float (&upper)[4], float (&r)[4]) noexcept
    {
        float c[4], d[4];
        c[0] = upper[0] / diag[0];
        d[0] = r[0] / diag[0];
        for (int i = 1; i < 4; ++i)
        {
            const float m = 1.0f / (diag[i] - lower[i] * c[i - 1]);
            c[i] = upper[i] * m;
            d[i] = (r[i] - lower[i] * d[i - 1]) * m;
        }
        r[3] = d[3];
        for (int i = 2; i >= 0; --i)
            r[i] = d[i] - c[i] * r[i + 1];
    }
}

void Ladder::prepare (double sampleRate) noexcept
{
    sr = sampleRate;
    const float g = std::tan (kPi * kFeedbackHpHz / (float) sr);
    hpG = g / (1.0f + g);
    reset();
}

void Ladder::reset() noexcept
{
    for (auto& v : s) v = 0.0f;
    hpState = 0.0f;
}

void Ladder::setModel (Model m) noexcept
{
    if (m != model)
    {
        model = m;
        reset();
    }
}

float Ladder::process (float x, float cutoffHz, float resonance) noexcept
{
    const bool diode = model == Model::Diode;
    const auto& A = diode ? kDiodeA : kTransA;

    const float fc = clampf (cutoffHz * (diode ? kDiodeTune : kTransTune), 10.0f, 0.42f * (float) sr);
    const float g = std::tan (kPi * fc / (float) sr);

    // (I - gA) y = g b u + s  ->  y = yb * u + ys
    float lower[4], diag[4], upper[4];
    for (int i = 0; i < 4; ++i)
    {
        lower[i] = i > 0 ? -g * A[i][i - 1] : 0.0f;
        diag[i]  = 1.0f - g * A[i][i];
        upper[i] = i < 3 ? -g * A[i][i + 1] : 0.0f;
    }
    float yb[4] = { g, 0.0f, 0.0f, 0.0f };
    float ys[4] = { s[0], s[1], s[2], s[3] };
    solveTridiagonal (lower, diag, upper, yb);
    solveTridiagonal (lower, diag, upper, ys);

    const float res = clampf (resonance, 0.0f, 1.0f);
    const float k = oscillationGain (model) * (diode ? kDiodeMaxFeedback : kTransMaxFeedback) * res;

    // Feedback = k * HP(y4) in the diode model (HP is linear in y4: a * y4 + c).
    const float a = diode ? (1.0f - hpG) : 1.0f;
    const float c = diode ? -(1.0f - hpG) * hpState : 0.0f;

    // Passband level drops as the feedback rises; make some of it up.
    const float makeup = diode ? 1.0f + 0.03f * k : 1.0f + 0.3f * k;
    const float in = x * makeup;

    float u = (in - k * (a * ys[3] + c)) / (1.0f + k * a * yb[3]);
    u = 1.25f * fastTanh (u * 0.8f);    // saturating ladder input

    float y[4];
    for (int i = 0; i < 4; ++i)
    {
        y[i] = yb[i] * u + ys[i];
        s[i] = 2.0f * y[i] - s[i];
        flushDenormal (s[i]);
    }

    if (diode)
    {
        const float v = (y[3] - hpState) * hpG;
        const float lp = v + hpState;
        hpState = lp + v;
        flushDenormal (hpState);
    }

    return y[3];
}
} // namespace acid
