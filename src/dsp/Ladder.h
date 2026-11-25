#pragma once

#include "DspCommon.h"

namespace acid
{
/** Zero-delay-feedback (TPT) 4-pole ladder filter with two circuit models:

     - Diode: coupled stages like the 303-style diode ladder. A high-pass sits in the
       feedback path so the resonance stays thin and squelchy, and the feedback is held
       just under the self-oscillation point.
     - Transistor: four identical buffered stages (classic transistor ladder), which can
       self-oscillate at full resonance.

    The linear 4x4 system is solved exactly every sample (tridiagonal solve), the
    feedback loop is resolved instantly and the ladder input is saturated. */
class Ladder
{
public:
    enum class Model { Diode, Transistor };

    void prepare (double sampleRate) noexcept;
    void reset() noexcept;
    void setModel (Model m) noexcept;
    Model getModel() const noexcept { return model; }

    /** cutoffHz = where the resonant peak sits, resonance 0..1. */
    float process (float x, float cutoffHz, float resonance) noexcept;

    /** Feedback gain at which the linear model would self-oscillate. */
    static float oscillationGain (Model m) noexcept { return m == Model::Diode ? 18.38f : 4.0f; }

private:
    double sr = 88200.0;
    Model model = Model::Diode;
    float s[4] {};          // integrator states
    float hpState = 0.0f;   // feedback high-pass (diode model)
    float hpG = 0.0f;
};
} // namespace acid
