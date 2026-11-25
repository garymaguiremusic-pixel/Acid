#include "Sequencer.h"

namespace acid
{
double Sequencer::stepLengthPpq (int rate) noexcept
{
    switch (rate)
    {
        case 0:  return 0.5;              // 1/8
        case 2:  return 1.0 / 6.0;        // 1/16 triplet
        case 3:  return 0.125;            // 1/32
        default: return 0.25;             // 1/16
    }
}

void Sequencer::reset() noexcept
{
    numPending = 0;
    gateOn = holdIntoNext = positionValid = false;
    lastStepTriggered = -1;
    currentStep = -1;
    soundingNote = -1;
    randomNextValid = false;
}

double Sequencer::boundary (int64_t n) const noexcept
{
    const double L = stepLengthPpq (set.rate);
    const double swingDelay = (2.0 / 3.0) * std::clamp ((double) set.swing, 0.0, 0.75) * L;
    const int64_t pair = n >= 0 ? n / 2 : (n - 1) / 2;
    const bool odd = (n - pair * 2) == 1;
    return (double) pair * 2.0 * L + (odd ? L + swingDelay : 0.0);
}

int Sequencer::indexForStep (int64_t n, int length) noexcept
{
    length = std::clamp (length, 1, kMaxSteps);
    const int64_t m = ((n % length) + length) % length;

    switch (set.direction)
    {
        case 1: return length - 1 - (int) m;                           // reverse
        case 2:                                                         // ping-pong
        {
            if (length < 2) return 0;
            const int64_t period = 2 * length - 2;
            const int64_t p = ((n % period) + period) % period;
            return p < length ? (int) p : (int) (period - p);
        }
        case 3:                                                         // random (stable per step)
        {
            if (randomNextValid && randomNextFor == n)
                return std::min (randomNext, length - 1);
            randomNextValid = true;
            randomNextFor = n;
            randomNext = (int) (rng.nextInt() % (uint32_t) length);
            return randomNext;
        }
        default: return (int) m;                                        // forward
    }
}

void Sequencer::addPending (const Pending& p) noexcept
{
    if (numPending < (int) std::size (pending))
        pending[numPending++] = p;
}

void Sequencer::triggerStep (const Pattern& pattern, int64_t n, double stepStart, double stepEnd, int keyTranspose)
{
    const int idx = indexForStep (n, pattern.length);
    const Step& st = pattern.steps[(size_t) idx];
    currentStep = idx;

    // Look ahead: does this step hold its gate into the next one?
    auto nextStepFor = [&] (int64_t k) -> const Step&
    {
        if (set.direction == 3)
        {
            // decide the next random index now so the lookahead and the real step agree
            const int nextIdx = (int) (rng.nextInt() % (uint32_t) std::clamp (pattern.length, 1, kMaxSteps));
            randomNextValid = true;
            randomNextFor = k;
            randomNext = nextIdx;
            return pattern.steps[(size_t) nextIdx];
        }
        return pattern.steps[(size_t) indexForStep (k, pattern.length)];
    };
    const Step& next = nextStepFor (n + 1);

    const bool wasHeld = holdIntoNext && gateOn;
    const double len = stepEnd - stepStart;
    const double gateLen = std::clamp ((double) set.gate, 0.05, 1.0) * len;

    auto emit = [this] (double ppq, SeqEvent::Type type, int note, bool accent, bool legato)
    {
        SeqEvent e;
        e.type = type; e.note = note; e.accent = accent; e.legato = legato;
        pushRaw (ppq, e);
    };

    auto closeGate = [&] (double ppq)
    {
        if (gateOn)
        {
            emit (ppq, SeqEvent::NoteOff, soundingNote, false, false);
            gateOn = false;
        }
        holdIntoNext = false;
    };

    if (st.gate == Gate::Rest || (st.gate == Gate::Tie && ! gateOn))
    {
        closeGate (stepStart);
        return;
    }

    const bool holds = (st.slide && next.gate != Gate::Rest) || next.gate == Gate::Tie;

    if (st.gate == Gate::Tie)
    {
        // extend the sounding note; it closes after this step unless held further
        holdIntoNext = holds;
        if (! holds)
            addPending ({ stepStart + gateLen, SeqEvent::NoteOff, soundingNote, false, noteGen });
        return;
    }

    // Note step
    const int note = std::clamp (36 + (int) st.note + 12 * (int) st.octave + set.transpose + keyTranspose, 0, 127);
    const int hits = wasHeld ? 1 : 1 + std::clamp ((int) st.ratchet, 0, kMaxRatchet);
    const double sub = len / hits;

    for (int h = 0; h < hits; ++h)
    {
        const double t = stepStart + sub * h;
        if (h == 0)
        {
            if (wasHeld)
            {
                emit (t, SeqEvent::NoteOn, note, st.accent, true);       // slide: glide, no retrigger
            }
            else
            {
                if (gateOn) emit (t, SeqEvent::NoteOff, soundingNote, false, false);
                emit (t, SeqEvent::NoteOn, note, st.accent, false);
            }
            ++noteGen;
            gateOn = true;
            soundingNote = note;
        }
        else
        {
            addPending ({ t, SeqEvent::NoteOn, note, st.accent, 0 });
        }

        const bool last = h == hits - 1;
        if (! (last && holds))
            addPending ({ t + (hits > 1 ? std::clamp ((double) set.gate, 0.05, 1.0) * sub : gateLen),
                          SeqEvent::NoteOff, note, false, noteGen + (uint32_t) h });
    }
    holdIntoNext = holds;
}

int Sequencer::stop (SeqEvent* out) noexcept
{
    numPending = 0;
    positionValid = false;
    holdIntoNext = false;
    if (! gateOn)
        return 0;
    gateOn = false;
    out[0] = SeqEvent { 0, SeqEvent::NoteOff, soundingNote, false, false };
    return 1;
}

int Sequencer::process (const Pattern& pattern, const SeqTransport& tr, int numSamples,
                        int keyTranspose, SeqEvent* out) noexcept
{
    if (! tr.running || tr.ppqPerSample <= 0.0 || numSamples <= 0)
        return stop (out);

    numRaw = 0;
    orderCounter = 0;
    const double start = tr.ppqStart;
    const double end = start + tr.ppqPerSample * numSamples;

    // A jump in the song position (loop, relocate): forget scheduled events.
    if (! positionValid || std::abs (start - lastPpqEnd) > tr.ppqPerSample * 4.0 + 1.0e-6)
    {
        numPending = 0;
        if (gateOn)
        {
            SeqEvent e { 0, SeqEvent::NoteOff, soundingNote, false, false };
            pushRaw (start, e);
            gateOn = false;
        }
        holdIntoNext = false;
        lastStepTriggered = -1;
        randomNextValid = false;
        positionValid = true;
    }
    lastPpqEnd = end;

    // first step whose boundary is at or after the block start
    const double L = stepLengthPpq (set.rate);
    int64_t n = (int64_t) std::floor (start / L) - 1;
    while (boundary (n) < start - 1.0e-9) ++n;

    // Merge step boundaries and pending events in time order.
    for (;;)
    {
        const double tStep = boundary (n);
        int best = -1;
        for (int i = 0; i < numPending; ++i)
            if (best < 0 || pending[i].ppq < pending[best].ppq)
                best = i;
        const double tPend = best >= 0 ? pending[best].ppq : 1.0e300;

        if (std::min (tStep, tPend) >= end - 1.0e-9)
            break;

        if (tPend <= tStep)   // offs at a boundary come before the new step
        {
            const Pending p = pending[best];
            pending[best] = pending[--numPending];

            if (p.type == SeqEvent::NoteOff)
            {
                if (gateOn && p.gen == noteGen)
                {
                    SeqEvent e { 0, SeqEvent::NoteOff, p.note, false, false };
                    pushRaw (std::max (start, p.ppq), e);
                    gateOn = false;
                }
            }
            else   // ratchet hit
            {
                if (gateOn)
                {
                    SeqEvent off { 0, SeqEvent::NoteOff, soundingNote, false, false };
                    pushRaw (std::max (start, p.ppq), off);
                }
                SeqEvent on { 0, SeqEvent::NoteOn, p.note, p.accent, false };
                pushRaw (std::max (start, p.ppq), on);
                gateOn = true;
                soundingNote = p.note;
                ++noteGen;
            }
        }
        else
        {
            if (n != lastStepTriggered)
            {
                triggerStep (pattern, n, tStep, boundary (n + 1), keyTranspose);
                lastStepTriggered = n;
            }
            ++n;
        }
    }

    // ppq -> sample offsets (stable order)
    int count = 0;
    for (int i = 0; i < numRaw && count < kMaxEvents; ++i)
    {
        auto e = raw[i].ev;
        const double rel = (raw[i].ppq - start) / tr.ppqPerSample;
        e.offset = std::clamp ((int) std::ceil (rel - 1.0e-6), 0, numSamples - 1);
        out[count++] = e;
    }
    // insertion sort by offset, keeping generation order for equal offsets
    for (int i = 1; i < count; ++i)
    {
        const auto e = out[i];
        int j = i - 1;
        while (j >= 0 && out[j].offset > e.offset) { out[j + 1] = out[j]; --j; }
        out[j + 1] = e;
    }
    return count;
}
} // namespace acid
