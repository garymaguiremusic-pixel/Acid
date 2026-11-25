#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;
using namespace acid;

//==============================================================================
AcidAudioProcessor::AcidAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "AcidState", createParameterLayout()),
      presets (*this)
{
    auto get = [this] (const String& id)
    {
        auto* ptr = apvts.getRawParameterValue (id);
        jassert (ptr != nullptr);
        return ptr;
    };

    using namespace ParamIDs;
    P.tune = get (tune);             P.volume = get (volume);        P.echoOn = get (echoOn);
    P.tubeOn = get (tubeOn);         P.wave = get (wave);            P.retro = get (retro);
    P.cutoffLow = get (cutoffLow);   P.cutoff = get (cutoff);        P.resonance = get (resonance);
    P.transistor = get (transistor); P.bass = get (bass);            P.treble = get (treble);
    P.drive = get (drive);           P.envMod = get (envMod);        P.decay = get (decay);
    P.accent = get (accent);         P.gate = get (gate);            P.envAttack = get (envAttack);
    P.accDecay = get (accDecay);     P.accSweep = get (accSweep);
    P.seqOn = get (seqOn);           P.keyMode = get (keyMode);      P.seqRate = get (seqRate);
    P.swing = get (swing);           P.slideTime = get (slideTime);  P.direction = get (direction);
    P.transpose = get (transpose);   P.pattern = get (pattern);
    P.oscBLevel = get (oscBLevel);   P.oscBTune = get (oscBTune);    P.oscBDetune = get (oscBDetune);
    P.unisonVoices = get (unisonVoices); P.unisonSpread = get (unisonSpread); P.unisonFree = get (unisonFree);
    P.ampAttack = get (ampAttack);   P.ampDecay = get (ampDecay);    P.ampRelease = get (ampRelease);
    P.echoSync = get (echoSync);     P.echoTime = get (echoTime);    P.echoMix = get (echoMix);
    P.echoFeedback = get (echoFeedback); P.echoLowCut = get (echoLowCut); P.echoHighCut = get (echoHighCut);
    P.echoWidth = get (echoWidth);
    P.tubeEdges[0] = get (tubeEdgeLow); P.tubeEdges[1] = get (tubeXover1);
    P.tubeEdges[2] = get (tubeXover2);  P.tubeEdges[3] = get (tubeEdgeHigh);
    P.tubeResonator = get (tubeResonator); P.tubeCharacter = get (tubeCharacter);
    P.tubeInput = get (tubeInput);   P.tubeOutput = get (tubeOutput); P.tubeMix = get (tubeMix);
    for (int b = 0; b < 3; ++b)
    {
        P.bandDrive[b] = get (band (b, "Drive"));
        P.bandGain[b]  = get (band (b, "Gain"));
        P.bandSolo[b]  = get (band (b, "Solo"));
    }
    for (int i = 0; i < 2; ++i)
    {
        P.lfoRate[i]   = get (lfo (i, "Rate"));   P.lfoDepth[i]  = get (lfo (i, "Depth"));
        P.lfoPhase[i]  = get (lfo (i, "Phase"));  P.lfoShape[i]  = get (lfo (i, "Shape"));
        P.lfoSync[i]   = get (lfo (i, "Sync"));   P.lfoMirror[i] = get (lfo (i, "Mirror"));
        P.lfoRetrig[i] = get (lfo (i, "Retrig")); P.lfoDest[i]   = get (lfo (i, "Dest"));
    }

    for (auto& pat : bank)
        pat = makeDefaultPattern();
    presets.loadFactory (0);    // a playable sound and pattern from the start

    apvts.addParameterListener (ParamIDs::pattern, this);
}

AcidAudioProcessor::~AcidAudioProcessor()
{
    apvts.removeParameterListener (ParamIDs::pattern, this);
}

bool AcidAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == AudioChannelSet::stereo();
}

void AcidAudioProcessor::parameterChanged (const String&, float)
{
    patternVersion.fetch_add (1);   // the shown pattern slot changed
}

//==============================================================================
void AcidAudioProcessor::prepareToPlay (double sr, int)
{
    sampleRate = sr;
    voice.prepare (sr);
    for (auto& l : lfos) l.prepare (sr);
    tone.prepare (sr);
    tube.prepare (sr);
    echo.prepare (sr);
    sequencer.reset();
    masterGain.reset (sr, 0.03);
    masterGain.setCurrentAndTargetValue (Decibels::decibelsToGain (P.volume->load()));
    updateSettings();
    setLatencySamples (voice.getLatencySamples() + MultibandTube::getLatencySamples());
    exchange.publish (bank);
    reset();
}

void AcidAudioProcessor::reset()
{
    voice.reset();
    tone.reset();
    tube.reset();
    echo.reset();
    sequencer.reset();
    numHeldNotes = numHeldKeys = 0;
    midiOutNote = -1;
    directVoiceActive = false;
}

//==============================================================================
void AcidAudioProcessor::updateSettings() noexcept
{
    VoiceSettings v;
    v.tuneSemis       = P.tune->load();
    v.waveMorph       = P.wave->load() > 0.5f ? 1.0f : 0.0f;
    v.retro           = P.retro->load() > 0.5f;
    v.cutoffHz        = cutoffToHz (P.cutoff->load());
    v.cutoffLowHz     = cutoffToHz (P.cutoffLow->load());
    v.resonance       = norm100 (P.resonance->load());
    v.envMod          = norm100 (P.envMod->load());
    v.decayMs         = decayToMs (P.decay->load());
    v.accDecayMs      = decayToMs (P.accDecay->load());
    v.envAttackMs     = envAttackToMs (P.envAttack->load());
    v.accent          = norm100 (P.accent->load());
    v.accSweep        = norm100 (P.accSweep->load());
    v.drive           = norm100 (P.drive->load());
    v.transistor      = P.transistor->load() > 0.5f;
    v.oscBLevel       = norm100 (P.oscBLevel->load());
    v.oscBTuneSemis   = P.oscBTune->load();
    v.oscBDetuneCents = P.oscBDetune->load();
    v.unisonVoices    = roundToInt (P.unisonVoices->load());
    v.unisonSpread    = norm100 (P.unisonSpread->load());
    v.unisonFree      = P.unisonFree->load() > 0.5f;
    v.ampAttackMs     = ampAttackToMs (P.ampAttack->load());
    v.ampHold         = ampDecayHolds (P.ampDecay->load());
    v.ampDecayMs      = ampDecayToMs (P.ampDecay->load());
    v.ampReleaseMs    = ampReleaseToMs (P.ampRelease->load());
    v.slideMs         = P.slideTime->load();
    voice.setSettings (v);

    SeqSettings s;
    s.rate      = roundToInt (P.seqRate->load());
    s.swing     = P.swing->load() / 100.0f;
    s.direction = roundToInt (P.direction->load());
    s.transpose = roundToInt (P.transpose->load());
    s.gate      = P.gate->load() / 100.0f;
    sequencer.setSettings (s);

    tone.setGains (P.bass->load(), P.treble->load());

    TubeSettings t;
    for (int i = 0; i < 4; ++i) t.edges[i] = P.tubeEdges[i]->load();
    for (int b = 0; b < 3; ++b)
    {
        t.drive[b] = norm100 (P.bandDrive[b]->load());
        t.bandGainDb[b] = P.bandGain[b]->load();
        t.solo[b] = P.bandSolo[b]->load() > 0.5f;
    }
    t.resonator = norm100 (P.tubeResonator->load());
    t.character = norm100 (P.tubeCharacter->load());
    t.inputDb   = P.tubeInput->load();
    t.outputDb  = P.tubeOutput->load();
    t.mix       = norm100 (P.tubeMix->load());
    t.active    = P.tubeOn->load() > 0.5f;
    tube.setSettings (t);

    EchoSettings e;
    e.on = P.echoOn->load() > 0.5f;
    if (P.echoSync->load() > 0.5f)
    {
        const auto& divs = echoDivisions();
        const double beats = divs[(size_t) divisionIndex (P.echoTime->load(), (int) divs.size())].beats;
        e.timeMs = (float) (beats * 60000.0 / jmax (20.0, bpm));
    }
    else
    {
        e.timeMs = echoFreeMs (P.echoTime->load());
    }
    e.mix       = norm100 (P.echoMix->load());
    e.feedback  = echoFeedbackGain (P.echoFeedback->load());
    e.lowCutHz  = echoLowCutHz (P.echoLowCut->load());
    e.highCutHz = echoHighCutHz (P.echoHighCut->load());
    e.width     = norm100 (P.echoWidth->load());
    echo.setSettings (e);

    for (int i = 0; i < 2; ++i)
    {
        lfos[i].setShape (roundToInt (P.lfoShape[i]->load()));
        lfos[i].setPhaseOffset (P.lfoPhase[i]->load());
        lfos[i].setMirror (P.lfoMirror[i]->load() > 0.5f);
        lfos[i].setRetrig (roundToInt (P.lfoRetrig[i]->load()));
        lfoDepthNow[i] = P.lfoDepth[i]->load() / 100.0f;
        lfoDestNow[i] = roundToInt (P.lfoDest[i]->load());
    }

    masterGain.setTargetValue (Decibels::decibelsToGain (P.volume->load(), -100.0f));
}

double AcidAudioProcessor::ppqAt (int sampleInBlock) const noexcept
{
    return (hostPlaying ? blockPpq : internalBlockStart) + ppqPerSample * sampleInBlock;
}

//==============================================================================
void AcidAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;
    const int total = buffer.getNumSamples();
    buffer.clear();
    if (total <= 0)
        return;

    // --- transport ------------------------------------------------------------------
    hostPlaying = false;
    bool ppqValid = false;
    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = jlimit (20.0, 400.0, *b);
            hostPlaying = pos->getIsPlaying();
            if (auto q = pos->getPpqPosition()) { blockPpq = *q; ppqValid = true; }
        }
    }
    if (hostPlaying && ! ppqValid)
        hostPlaying = false;     // can't sync without a position: use the internal clock
    ppqPerSample = bpm / 60.0 / sampleRate;

    const bool playRequest = internalPlayRequest.load();
    if (playRequest && ! internalPlayState)
        internalPpq = 0.0;       // the Play button starts the pattern from step 1
    internalPlayState = playRequest;
    internalBlockStart = internalPpq;

    updateSettings();

    const auto& patterns = exchange.read();
    const int slot = jlimit (0, kNumPatterns - 1, roundToInt (P.pattern->load()) - 1);
    const Pattern& pattern = patterns[(size_t) slot];

    MidiBuffer midiOut;
    float* left = buffer.getWritePointer (0);
    if (buffer.getNumChannels() < 2)
        return;
    float* right = buffer.getWritePointer (1);

    for (int start = 0; start < total; start += kChunk)
    {
        const int n = jmin (kChunk, total - start);
        processChunk (left + start, right + start, n, start, midi, midiOut, pattern);
    }

    // internal clock moves on while it is in use
    if (! hostPlaying && (internalPlayState || keyClockRunning))
        internalPpq = internalBlockStart + ppqPerSample * total;

    // --- effects -------------------------------------------------------------------
    tone.process (left, right, total);
    tube.process (left, right, total);
    echo.process (left, right, total);
    // master volume, then a gentle safety soft-clip above about -2 dBFS
    auto safety = [] (float x)
    {
        constexpr float knee = 0.8f;
        const float a = std::abs (x);
        if (a <= knee) return x;
        const float over = (a - knee) / (1.0f - knee);
        return std::copysign (knee + (1.0f - knee) * over / (1.0f + over), x);
    };
    for (int i = 0; i < total; ++i)
    {
        const float g = masterGain.getNextValue();
        left[i] = safety (left[i] * g);
        right[i] = safety (right[i] * g);
    }

    midi.swapWith (midiOut);

    // --- GUI state ----------------------------------------------------------------
    gui.running.store (internalRunning);
    gui.hostPlaying.store (hostPlaying);
    gui.currentStep.store (internalRunning ? sequencer.getCurrentStep() : -1);
    gui.bpm.store ((float) bpm);
    gui.filterEnv.store (voice.getFilterEnv());
    gui.accentCharge.store (voice.getAccentCharge());
    gui.cutoffHz.store (voice.getLastCutoffHz());
    gui.gateOn.store (voice.isGateOn());
    for (int i = 0; i < 2; ++i) gui.lfoPhase[i].store ((float) lfos[i].getPhase());
    for (int b = 0; b < 3; ++b) gui.bandLevel[b].store (tube.getBandLevel (b));
}

//==============================================================================
void AcidAudioProcessor::computeModulation (int start, int end) noexcept
{
    const int n = end - start;
    if (n <= 0)
        return;

    voiceMod = {};
    bool used[NumLfoDests] {};
    for (int i = 0; i < 2; ++i)
        if (std::abs (lfoDepthNow[i]) > 0.0f)
            used[jlimit (0, NumLfoDests - 1, lfoDestNow[i])] = true;

    if (used[DestCutoff]) { std::fill (modCutoff + start, modCutoff + end, 0.0f); voiceMod.cutoffOct  = modCutoff; }
    if (used[DestPitch])  { std::fill (modPitch + start, modPitch + end, 0.0f);   voiceMod.pitchSemis = modPitch; }
    if (used[DestWave])   { std::fill (modWave + start, modWave + end, 0.0f);     voiceMod.wave       = modWave; }
    if (used[DestReso])   { std::fill (modReso + start, modReso + end, 0.0f);     voiceMod.resonance  = modReso; }
    if (used[DestVolume]) { std::fill (modVolume + start, modVolume + end, 1.0f); voiceMod.volume     = modVolume; }

    const bool positionValid = hostPlaying || internalRunning;
    for (int i = 0; i < 2; ++i)
    {
        // rate: tempo divisions or Hz
        double cyclesPerSecond, cyclesPerBeat;
        if (P.lfoSync[i]->load() > 0.5f)
        {
            const auto& divs = lfoDivisions();
            const double beats = divs[(size_t) divisionIndex (P.lfoRate[i]->load(), (int) divs.size())].beats;
            cyclesPerBeat = 1.0 / beats;
            cyclesPerSecond = cyclesPerBeat * bpm / 60.0;
        }
        else
        {
            cyclesPerSecond = lfoFreeHz (P.lfoRate[i]->load());
            cyclesPerBeat = cyclesPerSecond * 60.0 / bpm;
        }

        float* buf = lfoBuf[i] + start;
        lfos[i].process (buf, n, cyclesPerSecond, cyclesPerBeat, positionValid, ppqAt (chunkOffset + start), ppqPerSample);

        const float d = lfoDepthNow[i];
        if (d == 0.0f)
            continue;

        switch (lfoDestNow[i])
        {
            case DestCutoff: for (int k = 0; k < n; ++k) modCutoff[start + k] += buf[k] * d * 4.0f;  break;   // +-4 octaves
            case DestPitch:  for (int k = 0; k < n; ++k) modPitch[start + k]  += buf[k] * d * 12.0f; break;   // +-12 st
            case DestWave:   for (int k = 0; k < n; ++k) modWave[start + k]   += buf[k] * d;         break;
            case DestReso:   for (int k = 0; k < n; ++k) modReso[start + k]   += buf[k] * d;         break;
            case DestVolume:
            {
                const float s = d > 0.0f ? 1.0f : -1.0f, a = std::abs (d);
                for (int k = 0; k < n; ++k)
                    modVolume[start + k] *= jmax (0.0f, 1.0f + 0.5f * a * (s * buf[k] - 1.0f));
                break;
            }
            default: break;
        }
    }
}

void AcidAudioProcessor::applyVoiceEvent (const SeqEvent& e, bool fromSequencer, MidiBuffer& midiOut, int sampleInBlock) noexcept
{
    if (e.type == SeqEvent::NoteOn)
    {
        voice.noteOn (e.note, e.accent, e.legato);
        if (! e.legato)
            for (auto& l : lfos) l.noteTriggered();
    }
    else
    {
        voice.noteOff();
    }

    if (! fromSequencer || ! midiOutEnabled.load())
        return;

    // sequencer -> host MIDI out (slides as overlapping notes)
    if (e.type == SeqEvent::NoteOn)
    {
        const int vel = e.accent ? 127 : 100;
        if (e.legato && midiOutNote >= 0)
        {
            if (midiOutNote != e.note)
            {
                midiOut.addEvent (MidiMessage::noteOn (1, e.note, (uint8) vel), sampleInBlock);
                midiOut.addEvent (MidiMessage::noteOff (1, midiOutNote), sampleInBlock);
            }
        }
        else
        {
            if (midiOutNote >= 0)
                midiOut.addEvent (MidiMessage::noteOff (1, midiOutNote), sampleInBlock);
            midiOut.addEvent (MidiMessage::noteOn (1, e.note, (uint8) vel), sampleInBlock);
        }
        midiOutNote = e.note;
    }
    else if (midiOutNote >= 0)
    {
        midiOut.addEvent (MidiMessage::noteOff (1, midiOutNote), sampleInBlock);
        midiOutNote = -1;
    }
}

void AcidAudioProcessor::processChunk (float* left, float* right, int numSamples, int chunkStart,
                                       const MidiBuffer& midiIn, MidiBuffer& midiOut, const Pattern& pattern) noexcept
{
    chunkOffset = chunkStart;
    int pos = 0;

    auto renderTo = [&] (int end)
    {
        end = jlimit (pos, numSamples, end);
        if (end <= pos) return;
        computeModulation (pos, end);
        voice.render (left + pos, right + pos, end - pos, voiceMod, pos);
        pos = end;
    };

    const bool seqEnabled = P.seqOn->load() > 0.5f;
    const bool keyMode = P.keyMode->load() > 0.5f;

    // Runs the sequencer over [from, to) of this chunk and renders up to `to`.
    auto runSequencer = [&] (int from, int to)
    {
        if (to <= from) return;

        const bool keyHeld = numHeldKeys > 0;
        const bool running = seqEnabled && (keyMode ? keyHeld : (hostPlaying || internalPlayState));
        keyClockRunning = keyMode && keyHeld && ! hostPlaying;
        internalRunning = running;

        if (running && directVoiceActive)
        {
            voice.noteOff();
            directVoiceActive = false;
            numHeldNotes = 0;
        }

        SeqTransport tr;
        tr.running = running;
        tr.ppqStart = ppqAt (chunkStart + from);
        tr.ppqPerSample = ppqPerSample;
        const int keyTranspose = keyMode && keyHeld ? heldKeys[(size_t) numHeldKeys - 1] - 60 : 0;

        if (seqEnabled || sequencer.isGateOn())
        {
            const int count = sequencer.process (pattern, tr, to - from, keyTranspose, seqEvents);
            for (int i = 0; i < count; ++i)
            {
                renderTo (from + seqEvents[i].offset);
                applyVoiceEvent (seqEvents[i], true, midiOut, chunkStart + from + seqEvents[i].offset);
            }
        }
        renderTo (to);
    };

    for (const auto meta : midiIn)
    {
        const int at = meta.samplePosition - chunkStart;
        if (at < 0) continue;
        if (at >= numSamples) break;

        runSequencer (pos, at);
        const auto msg = meta.getMessage();

        auto removeFrom = [] (std::array<int, 16>& stack, int& count, int note)
        {
            for (int i = 0; i < count; ++i)
                if (stack[(size_t) i] == note)
                {
                    for (int j = i; j < count - 1; ++j) stack[(size_t) j] = stack[(size_t) j + 1];
                    --count;
                    return true;
                }
            return false;
        };

        const bool sequencerOwnsNotes = seqEnabled && (keyMode || internalRunning);

        if (msg.isNoteOn())
        {
            const int note = msg.getNoteNumber();
            if (seqEnabled && keyMode)
            {
                removeFrom (heldKeys, numHeldKeys, note);
                if (numHeldKeys == 0 && ! hostPlaying)
                {
                    // the first key starts the pattern from step 1 right here
                    internalBlockStart = -ppqPerSample * (chunkStart + at);
                    sequencer.reset();
                }
                if (numHeldKeys < 16) heldKeys[(size_t) numHeldKeys++] = note;
            }
            else if (! sequencerOwnsNotes)
            {
                const bool legato = numHeldNotes > 0;
                removeFrom (heldNotes, numHeldNotes, note);
                if (numHeldNotes < 16) heldNotes[(size_t) numHeldNotes++] = note;
                applyVoiceEvent ({ at, SeqEvent::NoteOn, note, msg.getVelocity() >= 100, legato }, false, midiOut, chunkStart + at);
                directVoiceActive = true;
            }
        }
        else if (msg.isNoteOff())
        {
            const int note = msg.getNoteNumber();
            removeFrom (heldKeys, numHeldKeys, note);

            const bool wasTop = numHeldNotes > 0 && heldNotes[(size_t) numHeldNotes - 1] == note;
            if (removeFrom (heldNotes, numHeldNotes, note) && wasTop && directVoiceActive)
            {
                if (numHeldNotes > 0)
                    applyVoiceEvent ({ at, SeqEvent::NoteOn, heldNotes[(size_t) numHeldNotes - 1], false, true }, false, midiOut, chunkStart + at);
                else
                {
                    applyVoiceEvent ({ at, SeqEvent::NoteOff, note, false, false }, false, midiOut, chunkStart + at);
                    directVoiceActive = false;
                }
            }
        }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            numHeldNotes = numHeldKeys = 0;
            voice.noteOff();
            directVoiceActive = false;
        }
    }

    runSequencer (pos, numSamples);
}

//==============================================================================
// Patterns
//==============================================================================
int AcidAudioProcessor::getCurrentSlot() const
{
    return jlimit (0, kNumPatterns - 1, roundToInt (apvts.getRawParameterValue (ParamIDs::pattern)->load()) - 1);
}

void AcidAudioProcessor::editPattern (int slot, const std::function<void (Pattern&)>& edit, bool undoable)
{
    slot = jlimit (0, kNumPatterns - 1, slot);
    const Pattern before = bank[(size_t) slot];
    edit (bank[(size_t) slot]);
    if (bank[(size_t) slot] == before)
        return;
    if (undoable)
        patternUndo.push (slot, before);
    exchange.publish (bank);
    patternVersion.fetch_add (1);
}

void AcidAudioProcessor::setBank (const PatternBank& newBank)
{
    bank = newBank;
    exchange.publish (bank);
    patternVersion.fetch_add (1);
}

void AcidAudioProcessor::undoPattern()
{
    if (patternUndo.undoInto (bank))
    {
        exchange.publish (bank);
        patternVersion.fetch_add (1);
    }
}

void AcidAudioProcessor::redoPattern()
{
    if (patternUndo.redoInto (bank))
    {
        exchange.publish (bank);
        patternVersion.fetch_add (1);
    }
}

void AcidAudioProcessor::copyPattern()
{
    clipboard = bank[(size_t) getCurrentSlot()];
    clipboardValid = true;
}

void AcidAudioProcessor::pastePattern()
{
    if (clipboardValid)
        editPattern (getCurrentSlot(), [this] (Pattern& p) { p = clipboard; });
}

void AcidAudioProcessor::randomisePattern (PatternTools::RandomMode mode)
{
    const uint32_t seed = (uint32_t) Time::getMillisecondCounter() + randomSeed++;
    editPattern (getCurrentSlot(), [&] (Pattern& p) { PatternTools::randomise (p, mode, randomRoot, randomScale, seed); });
}

void AcidAudioProcessor::mutatePattern()
{
    const uint32_t seed = (uint32_t) Time::getMillisecondCounter() + randomSeed++;
    editPattern (getCurrentSlot(), [&] (Pattern& p) { PatternTools::mutate (p, randomRoot, randomScale, seed); });
}

void AcidAudioProcessor::rotatePattern (int direction)
{
    editPattern (getCurrentSlot(), [direction] (Pattern& p) { PatternTools::rotate (p, direction); });
}

void AcidAudioProcessor::clearPattern()
{
    editPattern (getCurrentSlot(), [] (Pattern& p) { PatternTools::clear (p); });
}

//==============================================================================
// Parameters
//==============================================================================
void AcidAudioProcessor::setParameterValue (const String& id, float value)
{
    if (auto* param = apvts.getParameter (id))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 (value));
        param->endChangeGesture();
    }
}

float AcidAudioProcessor::getParameterValue (const String& id) const
{
    if (auto* v = apvts.getRawParameterValue (id))
        return v->load();
    return 0.0f;
}

StringArray AcidAudioProcessor::tubeParameterIds()
{
    using namespace ParamIDs;
    StringArray ids { tubeEdgeLow, tubeXover1, tubeXover2, tubeEdgeHigh, tubeResonator, tubeCharacter, tubeInput, tubeOutput, tubeMix };
    for (int b = 0; b < 3; ++b)
        ids.addArray ({ band (b, "Drive"), band (b, "Gain") });
    return ids;
}

std::vector<float> AcidAudioProcessor::captureTube() const
{
    std::vector<float> v;
    for (auto& id : tubeParameterIds())
        v.push_back (getParameterValue (id));
    return v;
}

void AcidAudioProcessor::restoreTube (const std::vector<float>& v)
{
    const auto ids = tubeParameterIds();
    for (int i = 0; i < ids.size() && i < (int) v.size(); ++i)
        setParameterValue (ids[i], v[(size_t) i]);
}

void AcidAudioProcessor::pushTubeUndo()
{
    tubeUndo.push_back (captureTube());
    if (tubeUndo.size() > 50) tubeUndo.erase (tubeUndo.begin());
    tubeRedo.clear();
}

void AcidAudioProcessor::generateTube()
{
    pushTubeUndo();
    Random r;
    auto range = [&r] (float lo, float hi) { return lo + (hi - lo) * r.nextFloat(); };
    auto weighted = [&r] (float lo, float hi, float bias) { return lo + (hi - lo) * std::pow (r.nextFloat(), bias); };

    // crossovers at least an octave apart, in musical ranges
    const float lowEdge = range (30.0f, 120.0f);
    const float x1 = jmax (lowEdge * 2.0f, range (250.0f, 900.0f));
    const float x2 = jmax (x1 * 2.0f, range (1400.0f, 4000.0f));
    const float highEdge = jmax (x2 * 2.0f, range (7000.0f, 16000.0f));

    using namespace ParamIDs;
    setParameterValue (tubeEdgeLow, std::round (lowEdge));
    setParameterValue (tubeXover1, std::round (x1));
    setParameterValue (tubeXover2, std::round (x2));
    setParameterValue (tubeEdgeHigh, std::round (highEdge));
    for (int b = 0; b < 3; ++b)
    {
        setParameterValue (band (b, "Drive"), std::round (weighted (10.0f, 90.0f, 1.3f)));
        setParameterValue (band (b, "Gain"), std::round (range (-6.0f, 6.0f)));
    }
    setParameterValue (tubeResonator, std::round (weighted (0.0f, 70.0f, 1.8f)));
    setParameterValue (tubeCharacter, std::round (range (0.0f, 80.0f)));
    setParameterValue (tubeMix, std::round (range (60.0f, 100.0f)));
    setParameterValue (tubeOutput, std::round (range (-6.0f, 0.0f)));
}

void AcidAudioProcessor::initTube()
{
    pushTubeUndo();
    for (auto& id : tubeParameterIds())
        if (auto* param = apvts.getParameter (id))
            setParameterValue (id, param->convertFrom0to1 (param->getDefaultValue()));
}

void AcidAudioProcessor::undoTube()
{
    if (tubeUndo.empty()) return;
    tubeRedo.push_back (captureTube());
    restoreTube (tubeUndo.back());
    tubeUndo.pop_back();
}

void AcidAudioProcessor::redoTube()
{
    if (tubeRedo.empty()) return;
    tubeUndo.push_back (captureTube());
    restoreTube (tubeRedo.back());
    tubeRedo.pop_back();
}

void AcidAudioProcessor::initSound()
{
    for (auto* param : getParameters())
        if (auto* ranged = dynamic_cast<RangedAudioParameter*> (param))
            if (ranged->getParameterID() != ParamIDs::pattern)
            {
                ranged->beginChangeGesture();
                ranged->setValueNotifyingHost (ranged->getDefaultValue());
                ranged->endChangeGesture();
            }
    presets.setCurrentName ("Init");
}

//==============================================================================
// Programs = factory presets (for hosts that browse programs)
//==============================================================================
int AcidAudioProcessor::getNumPrograms()                  { return jmax (1, presets.getNumFactory()); }
int AcidAudioProcessor::getCurrentProgram()               { return jmax (0, presets.getCurrentFactoryIndex()); }
void AcidAudioProcessor::setCurrentProgram (int index)    { presets.loadFactory (index); }
const String AcidAudioProcessor::getProgramName (int i)   { return presets.getFactoryName (i); }

//==============================================================================
// State
//==============================================================================
void AcidAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("stateVersion", kStateVersion, nullptr);
    state.setProperty ("editorWidth", gui.editorWidth.load(), nullptr);
    state.setProperty ("lightTheme", gui.lightTheme.load(), nullptr);
    state.setProperty ("page", gui.page.load(), nullptr);
    state.setProperty ("presetName", presets.getCurrentName(), nullptr);
    state.setProperty ("randomRoot", randomRoot, nullptr);
    state.setProperty ("randomScale", randomScale, nullptr);
    state.setProperty ("midiOut", midiOutEnabled.load(), nullptr);

    ValueTree patternsTree ("Patterns");
    for (auto& pat : bank)
    {
        ValueTree pt ("Pattern");
        pt.setProperty ("data", String (patternToString (pat)), nullptr);
        patternsTree.appendChild (pt, nullptr);
    }
    state.removeChild (state.getChildWithName ("Patterns"), nullptr);
    state.appendChild (patternsTree, nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void AcidAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;

    auto state = ValueTree::fromXml (*xml);
    // (stateVersion is 1 so far; future versions migrate here)
    gui.editorWidth.store ((int) state.getProperty ("editorWidth", 0));
    gui.lightTheme.store ((bool) state.getProperty ("lightTheme", false));
    gui.page.store ((int) state.getProperty ("page", 0));
    presets.setCurrentName (state.getProperty ("presetName", "Init").toString());
    randomRoot = (int) state.getProperty ("randomRoot", randomRoot);
    randomScale = (int) state.getProperty ("randomScale", randomScale);
    midiOutEnabled.store ((bool) state.getProperty ("midiOut", true));

    PatternBank newBank;
    for (auto& p : newBank) p = makeDefaultPattern();
    auto patternsTree = state.getChildWithName ("Patterns");
    for (int i = 0; i < patternsTree.getNumChildren() && i < kNumPatterns; ++i)
        patternFromString (patternsTree.getChild (i).getProperty ("data").toString().toStdString(), newBank[(size_t) i]);
    state.removeChild (patternsTree, nullptr);

    apvts.replaceState (state);

    // Push every restored value explicitly: replaceState skips parameters whose stored
    // value looks unchanged, which can leave a toggle at an in-between value.
    for (auto* param : getParameters())
        if (auto* ranged = dynamic_cast<RangedAudioParameter*> (param))
        {
            const auto child = state.getChildWithProperty ("id", ranged->getParameterID());
            const float v = child.isValid() ? (float) child.getProperty ("value") : ranged->convertFrom0to1 (ranged->getDefaultValue());
            ranged->setValueNotifyingHost (ranged->convertTo0to1 (v));
        }

    setBank (newBank);
}

//==============================================================================
AudioProcessorEditor* AcidAudioProcessor::createEditor()
{
    return new AcidAudioProcessorEditor (*this);
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AcidAudioProcessor();
}
