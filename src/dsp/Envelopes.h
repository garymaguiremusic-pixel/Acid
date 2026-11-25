#pragma once

#include "DspCommon.h"

namespace acid
{
//==============================================================================
/** Filter envelope: short linear attack, then exponential decay to zero. Retriggered
    on every new (non-slid) note; accented notes use the accent decay time. */
class FilterEnvelope
{
public:
    void prepare (double sampleRate) noexcept { sr = sampleRate; reset(); }
    void reset() noexcept { value = 0.0f; attacking = false; }

    void trigger (float attackMs, float decayMs) noexcept
    {
        attackInc = 1.0f / std::max (1.0f, attackMs * 0.001f * (float) sr);
        decayCoef = std::exp (-4.6f / std::max (1.0f, decayMs * 0.001f * (float) sr));   // ~ -40 dB after decayMs
        attacking = true;
    }

    float process() noexcept
    {
        if (attacking)
        {
            value += attackInc;
            if (value >= 1.0f) { value = 1.0f; attacking = false; }
        }
        else
        {
            value *= decayCoef;
            flushDenormal (value);
        }
        return value;
    }

    float getValue() const noexcept { return value; }

private:
    double sr = 44100.0;
    float value = 0.0f, attackInc = 1.0f, decayCoef = 0.999f;
    bool attacking = false;
};

//==============================================================================
/** Amp envelope: linear attack, exponential decay while the gate is held (decay at
    the top of its range holds), exponential release when the gate closes. */
class AmpEnvelope
{
public:
    void prepare (double sampleRate) noexcept { sr = sampleRate; reset(); }
    void reset() noexcept { value = 0.0f; stage = Stage::Idle; }

    void setTimes (float attackMs, float decayMs, bool holdForever, float releaseMs) noexcept
    {
        attackInc = 1.0f / std::max (1.0f, attackMs * 0.001f * (float) sr);
        decayCoef = holdForever ? 1.0f : std::exp (-4.6f / std::max (1.0f, decayMs * 0.001f * (float) sr));
        releaseCoef = std::exp (-4.6f / std::max (1.0f, releaseMs * 0.001f * (float) sr));
    }

    void gateOn() noexcept   { stage = Stage::Attack; }
    void gateOff() noexcept  { if (stage != Stage::Idle) stage = Stage::Release; }
    bool isActive() const noexcept { return stage != Stage::Idle; }

    float process() noexcept
    {
        switch (stage)
        {
            case Stage::Attack:
                value += attackInc;
                if (value >= 1.0f) { value = 1.0f; stage = Stage::Decay; }
                break;
            case Stage::Decay:
                value *= decayCoef;
                break;
            case Stage::Release:
                value *= releaseCoef;
                if (value < 1.0e-5f) { value = 0.0f; stage = Stage::Idle; }
                break;
            case Stage::Idle:
                break;
        }
        return value;
    }

private:
    enum class Stage { Idle, Attack, Decay, Release };
    double sr = 44100.0;
    float value = 0.0f, attackInc = 1.0f, decayCoef = 1.0f, releaseCoef = 0.99f;
    Stage stage = Stage::Idle;
};

//==============================================================================
/** The accent "wow" circuit. Each accented note sends a pulse (the accent envelope
    times the accent amount) into a capacitor that charges faster than it discharges.
    With Sweep at 0 the output follows the pulse exactly; turning Sweep up smooths the
    pulse and lets charge build up over consecutive accents, so a run of accented
    notes pushes the cutoff higher and higher. */
class AccentCircuit
{
public:
    void prepare (double sampleRate) noexcept { sr = sampleRate; reset(); setSweep (sweep); }
    void reset() noexcept { cap = 0.0f; }

    void setSweep (float sweep01) noexcept
    {
        sweep = clampf (sweep01, 0.0f, 1.0f);
        chargeCoef    = onePoleCoef (2.0f + sweep * 90.0f, sr);
        dischargeCoef = onePoleCoef (4.0f + sweep * 900.0f, sr);
        makeup = 1.0f + sweep * 1.2f;
    }

    float process (float pulse) noexcept
    {
        if (sweep <= 0.0f)
        {
            cap = pulse;
            return pulse;
        }
        cap += (pulse - cap) * (pulse > cap ? chargeCoef : dischargeCoef);
        flushDenormal (cap);
        return cap * makeup;
    }

    float getCharge() const noexcept { return cap; }

private:
    double sr = 44100.0;
    float sweep = 0.0f, cap = 0.0f, chargeCoef = 1.0f, dischargeCoef = 1.0f, makeup = 1.0f;
};
} // namespace acid
