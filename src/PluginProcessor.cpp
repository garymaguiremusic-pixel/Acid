#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "arp/Scales.h"
#include "presets/FactoryPresets.h"
#include <limits>

using namespace juce;
using namespace acid;

namespace
{
    const Identifier sequencePathID { "sequencePath" };
    const Identifier seedID         { "randomSeed" };
}

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
    P.transistor = get (transistor); P.bass = get (bass);
    P.filterVelLink = get (filterVelLink); P.filterVelAmount = get (filterVelAmount);
    P.filterMode = get (filterMode); P.oscALock = get (oscALock); P.oscBLock = get (oscBLock);
    P.engine = get (engine); P.oscAOctave = get (oscAOctave); P.oscBOctave = get (oscBOctave);
    P.b3Wave = get (b3Wave); P.b3Tuning = get (b3Tuning); P.b3Octave = get (b3Octave); P.b3Cutoff = get (b3Cutoff);
    P.b3Resonance = get (b3Resonance); P.b3EnvMod = get (b3EnvMod); P.b3Decay = get (b3Decay); P.b3Accent = get (b3Accent);
    P.b3Volume = get (b3Volume); P.b3Model = get (b3Model); P.b3AccurateGate = get (b3AccurateGate);
    P.b3VcaNoise = get (b3VcaNoise); P.b3Vibrato = get (b3Vibrato); P.b3PhaseReset = get (b3PhaseReset);
    P.b3SlowSlide = get (b3SlowSlide); P.b3WeakAccentCap = get (b3WeakAccentCap); P.b3InfSustain = get (b3InfSustain);
    P.b3SoftAccent = get (b3SoftAccent);
    P.distPageOn = get (distPageOn); P.distOn = get (distOn); P.distMode = get (distMode);
    P.distDrive = get (distDrive); P.distAmount = get (distAmount);            P.treble = get (treble);
    P.drive = get (drive);           P.envMod = get (envMod);        P.decay = get (decay);
    P.accent = get (accent);         P.envAttack = get (envAttack);
    P.accDecay = get (accDecay);     P.accSweep = get (accSweep);    P.slideTime = get (slideTime);
    P.arpOn = get (arpOn);           P.arpMode = get (arpMode);      P.arpRate = get (arpRate);
    P.arpRateType = get (arpRateType); P.arpGate = get (arpGate);    P.arpSwing = get (arpSwing);
    P.arpOctave = get (arpOctave);   P.velMode = get (velMode);      P.bpm = get (ParamIDs::bpm);
    P.autoPlay = get (autoPlay);     P.midiOut = get (midiOut);      P.seqLength = get (seqLength);
    P.seqEnd = get (seqEnd);         P.scaleKey = get (scaleKey);    P.scale = get (ParamIDs::scale);
    P.scaleLock = get (scaleLock);
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

    presets.loadFactory (0);    // a playable sound and pattern from the start
    patternModel.clearHistory();

    apvts.addParameterListener (ParamIDs::scaleKey, this);
    apvts.addParameterListener (ParamIDs::scale, this);
    apvts.addParameterListener (ParamIDs::scaleLock, this);
}

AcidAudioProcessor::~AcidAudioProcessor()
{
    cancelPendingUpdate();
    apvts.removeParameterListener (ParamIDs::scaleLock, this);
    apvts.removeParameterListener (ParamIDs::scale, this);
    apvts.removeParameterListener (ParamIDs::scaleKey, this);
}

bool AcidAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == AudioChannelSet::stereo();
}

//==============================================================================
void AcidAudioProcessor::prepareToPlay (double sr, int)
{
    sampleRate = sr;
    voice.prepare (sr);
    voice303.prepare (sr, kChunk);
    distortion.prepare (sr);
    for (auto& l : lfos) l.prepare (sr);
    tone.prepare (sr);
    tube.prepare (sr);
    echo.prepare (sr);
    arp.prepare (sr);
    masterGain.reset (sr, 0.03);
    masterGain.setCurrentAndTargetValue (Decibels::decibelsToGain (P.volume->load()));
    updateSettings();
    setLatencySamples (voice.getLatencySamples() + Distortion::getLatencySamples() + MultibandTube::getLatencySamples());
    reset();
}

void AcidAudioProcessor::reset()
{
    voice.reset();
    voice303.reset();
    distortion.reset();
    tone.reset();
    tube.reset();
    echo.reset();
    arp.reset();
    midiWriter.reset();
    numHeldNotes = 0;
    voiceArpId = 0;
}

//==============================================================================
// Sequencer helpers (message thread)
//==============================================================================
void AcidAudioProcessor::parameterChanged (const String&, float)
{
    // Key / Scale / Scale Lock changed: convert the step numbers (see syncPatternUnits)
    if (! restoringState.load())
        triggerAsyncUpdate();
}

void AcidAudioProcessor::handleAsyncUpdate()
{
    syncPatternUnits();
}

void AcidAudioProcessor::syncPatternUnits()
{
    // With Scale Lock on, the step numbers count notes of the scale; off, they are semitones.
    // When Scale Lock is switched (or an old pattern comes back), convert the numbers so the
    // pattern keeps its notes.
    const bool wantSteps = P.scaleLock->load() > 0.5f;
    auto p = patternModel.get();
    if (p.scaleSteps == wantSteps)
        return;

    const int scaleIndex = roundToInt (P.scale->load());
    auto convert = [&] (int t)
    {
        return jlimit (-24, 24, wantSteps ? as::arp::semitonesToScaleSteps (t, scaleIndex) : as::arp::scaleStepsToSemitones (t, scaleIndex));
    };
    for (auto& step : p.steps)
        step.transpose = convert (step.transpose);

    decltype (p.notePool) pool {};
    for (int t = -24; t <= 24; ++t)
        if (const int c = p.poolCount (t); c > 0)
        {
            auto& slot = pool[(size_t) (convert (t) + 24)];
            slot = (std::uint8_t) jmin (255, slot + c);
        }
    p.notePool = pool;
    p.scaleSteps = wantSteps;
    p.key = roundToInt (P.scaleKey->load());
    p.scale = scaleIndex;
    patternModel.set (p, false);
}

void AcidAudioProcessor::setPattern (const as::arp::Pattern& pattern, int length)
{
    // preset patterns are written in semitones: convert to scale steps if Scale Lock is on
    auto p = pattern;
    p.scaleSteps = false;
    p.key = roundToInt (P.scaleKey->load());
    p.scale = roundToInt (P.scale->load());
    patternModel.set (p, true);
    syncPatternUnits();

    length = jlimit (1, as::kMaxSteps, length);
    setParameterValue (ParamIDs::seqLength, length > 16 ? 1.0f : 0.0f);
    setParameterValue (ParamIDs::seqEnd, (length == 16 || length == 32) ? 0.0f : (float) length);
}

void AcidAudioProcessor::restorePattern (const ValueTree& tree)
{
    auto copy = patternModel.get();
    as::arp::PatternExchange scratchExchange;
    as::arp::PatternModel scratch (scratchExchange);
    scratch.fromValueTree (tree);
    copy = scratch.get();
    patternModel.set (copy, true);
    syncPatternUnits();
}

as::arp::RandomiseSettings AcidAudioProcessor::readRandomiseSettings() const
{
    auto value = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    using namespace ParamIDs;

    as::arp::RandomiseSettings r;
    r.density            = value (rndDensity) * 0.01f;
    r.range              = roundToInt (value (rndRange));
    r.velocityMin        = roundToInt (value (rndVelMin));
    r.velocityMax        = roundToInt (value (rndVelMax));
    r.lockPitch          = value (rndLockPitch) > 0.5f;
    r.lockVelocity       = value (rndLockVel) > 0.5f;
    r.lockRests          = value (rndLockRest) > 0.5f;
    r.ratchetProbability = value (rndRatchet) * 0.01f;
    r.lockRatchets       = value (rndLockRatchet) > 0.5f;
    r.reverseProbability = 0.0f;            // a synth has no sample to play backwards
    r.lockReverse        = true;
    r.accentProbability  = value (rndAccent) * 0.01f;
    r.lockAccents        = value (rndLockAccent) > 0.5f;
    r.glideProbability   = value (rndGlide) * 0.01f;
    r.lockGlides         = value (rndLockGlide) > 0.5f;
    r.fileNotes          = value (rndFileNotes) > 0.5f;
    r.scaleLock          = value (ParamIDs::scaleLock) > 0.5f;
    r.scaleKey           = roundToInt (value (ParamIDs::scaleKey));
    r.scaleIndex         = roundToInt (value (ParamIDs::scale));
    return r;
}

void AcidAudioProcessor::randomisePattern (std::optional<uint32> seed)
{
    randomSeed = seed.has_value() ? *seed : (uint32) Random::getSystemRandom().nextInt (999999999) + 1u;
    syncPatternUnits();
    patternModel.set (as::arp::randomisePattern (patternModel.get(), readRandomiseSettings(), randomSeed), true);
}

as::arp::Pattern AcidAudioProcessor::makeEmptyPattern() const
{
    as::arp::Pattern p;
    p.notePool = patternModel.get().notePool;      // Clear keeps the imported file's notes for File Notes
    p.scaleSteps = P.scaleLock->load() > 0.5f;
    p.key = roundToInt (P.scaleKey->load());
    p.scale = roundToInt (P.scale->load());
    return p;
}

as::import::ImportOptions AcidAudioProcessor::makeImportOptions() const
{
    as::import::ImportOptions o;
    o.stepLength = as::arp::stepLengthInQuarters (roundToInt (P.arpRate->load()), (as::RateType) roundToInt (P.arpRateType->load()));
    o.startBar = importChoices.startBar;
    o.topNote = importChoices.topNote;
    o.scaleSteps = P.scaleLock->load() > 0.5f;
    o.key = roundToInt (P.scaleKey->load());
    o.scale = roundToInt (P.scale->load());
    o.baseNote = kSequenceBaseNote;
    return o;
}

int AcidAudioProcessor::applyImport (const std::vector<as::import::ImportedNote>& notes, bool lockToScale)
{
    auto options = makeImportOptions();
    options.snapToScale = lockToScale;
    const auto result = as::import::notesToPattern (notes, options);
    if (result.notesUsed == 0)
        return 0;

    patternModel.set (result.pattern, true);
    setParameterValue (ParamIDs::seqLength, result.length > 16 ? 1.0f : 0.0f);
    setParameterValue (ParamIDs::seqEnd, (float) result.end);
    setParameterValue (ParamIDs::arpMode, (float) as::ArpMode::Step);     // the steps play the imported line
    setParameterValue (ParamIDs::arpOn, 1.0f);
    return result.notesUsed;
}

as::exporter::ExportRequest AcidAudioProcessor::buildExportRequest() const
{
    auto value = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    as::exporter::ExportRequest r;
    r.settings = readArpSettings();
    r.settings.enabled = true;
    r.pattern = patternModel.get();
    r.bpm = getTempo();

    static constexpr int barChoices[] = { 1, 2, 4, 8 };
    r.bars = barChoices[jlimit (0, 3, roundToInt (value (ParamIDs::exportBars)))];

    // Auto: the keys you last played; with none yet, the Key's root in the bass octave
    const int chosenRoot = roundToInt (value (ParamIDs::exportRoot));
    const auto held = getLastHeldNotes();
    const bool autoRoot = chosenRoot < 0;
    const int root = autoRoot ? kSequenceBaseNote + r.settings.scaleKey : chosenRoot;

    if (r.settings.mode == as::ArpMode::Step)
    {
        r.input = { { autoRoot && ! held.empty() ? held.back().note : root, 100 } };
    }
    else
    {
        r.input = held;
        if (r.input.empty())
            for (auto interval : as::exporter::chordIntervals (roundToInt (value (ParamIDs::exportChord))))
                if (root + interval <= 127)
                    r.input.push_back ({ root + interval, 100 });
    }
    return r;
}

File AcidAudioProcessor::exportMidiToTempFile() const
{
    const auto request = buildExportRequest();
    return as::exporter::writeToTempFile (as::exporter::renderPattern (request), as::exporter::suggestedName (request));
}

void AcidAudioProcessor::publishHeldNotes() noexcept
{
    const int count = arp.getNumHeld();
    if (count == 0)
        return;     // keep the last keys that were played

    const auto* held = arp.getHeldNotes();
    bool same = count == lastPublishedCount;
    for (int i = 0; same && i < count; ++i)
        same = held[i].note == lastPublishedHeld[(size_t) i].note && held[i].velocity == lastPublishedHeld[(size_t) i].velocity;
    if (same)
        return;

    const auto v = heldVersion.load (std::memory_order_relaxed);
    heldVersion.store (v + 1, std::memory_order_release);        // odd: writing
    std::atomic_thread_fence (std::memory_order_release);
    for (int i = 0; i < count; ++i)
    {
        heldNotesPublished[(size_t) i].store (held[i].note, std::memory_order_relaxed);
        heldVelocities[(size_t) i].store (held[i].velocity, std::memory_order_relaxed);
        lastPublishedHeld[(size_t) i] = held[i];
    }
    heldCount.store (count, std::memory_order_relaxed);
    lastPublishedCount = count;
    heldVersion.store (v + 2, std::memory_order_release);        // even: done
}

std::vector<as::arp::HeldNote> AcidAudioProcessor::getLastHeldNotes() const
{
    std::vector<as::arp::HeldNote> result;
    for (int attempt = 0; attempt < 100; ++attempt)
    {
        const auto v1 = heldVersion.load (std::memory_order_acquire);
        if ((v1 & 1u) != 0)
            continue;

        result.clear();
        const int count = jlimit (0, as::arp::kMaxHeld, heldCount.load (std::memory_order_relaxed));
        for (int i = 0; i < count; ++i)
            result.push_back ({ heldNotesPublished[(size_t) i].load (std::memory_order_relaxed),
                                heldVelocities[(size_t) i].load (std::memory_order_relaxed) });

        std::atomic_thread_fence (std::memory_order_acquire);
        if (heldVersion.load (std::memory_order_relaxed) == v1)
            return result;
    }
    return result;
}

//==============================================================================
// Audio
//==============================================================================
as::arp::ArpSettings AcidAudioProcessor::readArpSettings() const noexcept
{
    as::arp::ArpSettings s;
    s.enabled      = P.arpOn->load() > 0.5f;
    s.mode         = (as::ArpMode) roundToInt (P.arpMode->load());
    s.rateIndex    = roundToInt (P.arpRate->load());
    s.rateType     = (as::RateType) roundToInt (P.arpRateType->load());
    s.gate         = P.arpGate->load() * 0.01f;
    s.swing        = P.arpSwing->load() * 0.01f;
    s.octaves      = roundToInt (P.arpOctave->load());
    s.velocityMode = (as::VelocityMode) roundToInt (P.velMode->load());
    s.retrigger    = true;          // mono synth: a new step retriggers, a Tie glides

    const int length = roundToInt (P.seqLength->load()) == 0 ? 16 : 32;
    s.sequenceLength = as::arp::effectiveLength (length, roundToInt (P.seqEnd->load()));

    s.scaleKey   = roundToInt (P.scaleKey->load());
    s.scaleIndex = roundToInt (P.scale->load());
    s.followKey  = P.scaleLock->load() > 0.5f;
    s.autoPlay   = P.autoPlay->load() > 0.5f;
    s.autoNote   = kSequenceBaseNote + s.scaleKey;     // the Key's root in the bass octave
    return s;
}

as::arp::TransportInfo AcidAudioProcessor::readTransport() noexcept
{
    as::arp::TransportInfo t;
    t.bpm = P.bpm->load();
    bool hostTempo = false;
    hostPlaying = false;

    if (auto* ph = getPlayHead())
    {
        if (const auto position = ph->getPosition())
        {
            if (const auto hostBpm = position->getBpm(); hostBpm.hasValue() && *hostBpm > 0.0)
            {
                t.bpm = *hostBpm;
                hostTempo = true;
            }

            if (position->getIsPlaying())
            {
                if (const auto ppq = position->getPpqPosition())
                {
                    t.hostPlaying = true;
                    t.ppqAtBlockStart = *ppq;

                    if (const auto loop = position->getLoopPoints(); loop.hasValue() && position->getIsLooping())
                    {
                        t.looping = true;
                        t.loopStart = loop->ppqStart;
                        t.loopEnd = loop->ppqEnd;
                    }
                }
            }
        }
    }

    bpm = jlimit (20.0, 400.0, t.bpm);
    hostPlaying = t.hostPlaying;
    blockPpq = t.ppqAtBlockStart;
    ppqPerSample = bpm / 60.0 / sampleRate;
    gui.bpm.store ((float) bpm);
    gui.hostTempo.store (hostTempo);
    return t;
}

void AcidAudioProcessor::updateSettings() noexcept
{
    VoiceSettings v;
    v.tuneSemis       = P.tune->load() + 12.0f * P.oscAOctave->load();
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
    v.filterMode      = roundToInt (P.filterMode->load());
    v.oscALock        = P.oscALock->load() > 0.5f;
    v.oscBLock        = P.oscBLock->load() > 0.5f;
    v.velLink         = P.filterVelLink->load() > 0.5f;
    v.velAmount       = norm100 (P.filterVelAmount->load());
    v.oscBLevel       = norm100 (P.oscBLevel->load());
    v.oscBTuneSemis   = P.oscBTune->load() + 12.0f * P.oscBOctave->load();
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

    Settings303 b;
    b.wave          = roundToInt (P.b3Wave->load());
    b.tuningSemis   = P.b3Tuning->load() + P.tune->load();
    b.octave        = roundToInt (P.b3Octave->load());
    b.cutoff        = norm100 (P.b3Cutoff->load());
    b.resonance     = norm100 (P.b3Resonance->load());
    b.envMod        = norm100 (P.b3EnvMod->load());
    b.decay         = norm100 (P.b3Decay->load());
    b.accent        = norm100 (P.b3Accent->load());
    b.volume        = norm100 (P.b3Volume->load());
    b.slideMs       = P.slideTime->load();
    b.model         = roundToInt (P.b3Model->load());
    b.accurateGate  = P.b3AccurateGate->load() > 0.5f;
    b.vcaNoise      = P.b3VcaNoise->load() > 0.5f;
    b.vibrato       = P.b3Vibrato->load() > 0.5f;
    b.phaseReset    = P.b3PhaseReset->load() > 0.5f;
    b.slowSlide     = P.b3SlowSlide->load() > 0.5f;
    b.weakAccentCap = P.b3WeakAccentCap->load() > 0.5f;
    b.infSustain    = P.b3InfSustain->load() > 0.5f;
    b.softAccent    = P.b3SoftAccent->load() > 0.5f;
    b.oscBLevel       = v.oscBLevel;           // the Layers page plays on the 303 too
    b.oscBTuneSemis   = v.oscBTuneSemis;
    b.oscBDetuneCents = v.oscBDetuneCents;
    b.oscBLock        = v.oscBLock;
    b.unisonVoices    = v.unisonVoices;
    b.unisonSpread    = v.unisonSpread;
    b.unisonFree      = v.unisonFree;
    voice303.setSettings (b);

    const bool distPage = P.distPageOn->load() > 0.5f;
    DistortionSettings d;
    d.on      = distPage && P.distOn->load() > 0.5f;
    d.mode    = roundToInt (P.distMode->load());
    d.drive   = norm100 (P.distDrive->load());
    d.distort = norm100 (P.distAmount->load());
    distortion.setSettings (d);

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
    t.active    = P.tubeOn->load() > 0.5f && P.distPageOn->load() > 0.5f;
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


//==============================================================================
void AcidAudioProcessor::computeModulation (int sampleInBlock, int n) noexcept
{
    voiceMod = {};
    bool used[NumLfoDests] {};
    for (int i = 0; i < 2; ++i)
        if (std::abs (lfoDepthNow[i]) > 0.0f)
            used[jlimit (0, NumLfoDests - 1, lfoDestNow[i])] = true;

    if (used[DestCutoff]) { std::fill (modCutoff, modCutoff + n, 0.0f); voiceMod.cutoffOct  = modCutoff; }
    if (used[DestPitch])  { std::fill (modPitch, modPitch + n, 0.0f);   voiceMod.pitchSemis = modPitch; }
    if (used[DestWave])   { std::fill (modWave, modWave + n, 0.0f);     voiceMod.wave       = modWave; }
    if (used[DestReso])   { std::fill (modReso, modReso + n, 0.0f);     voiceMod.resonance  = modReso; }
    if (used[DestVolume]) { std::fill (modVolume, modVolume + n, 1.0f); voiceMod.volume     = modVolume; }

    for (int i = 0; i < 2; ++i)
    {
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

        float* buf = lfoBuf[i];
        lfos[i].process (buf, n, cyclesPerSecond, cyclesPerBeat, hostPlaying, blockPpq + ppqPerSample * sampleInBlock, ppqPerSample);

        const float d = lfoDepthNow[i];
        if (d == 0.0f)
            continue;

        switch (lfoDestNow[i])
        {
            case DestCutoff: for (int k = 0; k < n; ++k) modCutoff[k] += buf[k] * d * 4.0f;  break;   // +-4 octaves
            case DestPitch:  for (int k = 0; k < n; ++k) modPitch[k]  += buf[k] * d * 12.0f; break;   // +-12 st
            case DestWave:   for (int k = 0; k < n; ++k) modWave[k]   += buf[k] * d;         break;
            case DestReso:   for (int k = 0; k < n; ++k) modReso[k]   += buf[k] * d;         break;
            case DestVolume:
            {
                const float s = d > 0.0f ? 1.0f : -1.0f, a = std::abs (d);
                for (int k = 0; k < n; ++k)
                    modVolume[k] *= jmax (0.0f, 1.0f + 0.5f * a * (s * buf[k] - 1.0f));
                break;
            }
            default: break;
        }
    }
}

void AcidAudioProcessor::renderVoice (float* left, float* right, int from, int to) noexcept
{
    // the selected synth always renders; the other one only while its last note rings out
    for (int pos = from; pos < to; pos += kChunk)
    {
        const int n = jmin (kChunk, to - pos);
        const bool acid = activeEngine == 0 || voice.isSounding();
        const bool b303 = activeEngine == 1 || voice303.isSounding();

        if (acid || b303)
            computeModulation (pos, n);       // the LFOs move both synths

        if (acid)
        {
            voice.render (left + pos, right + pos, n, voiceMod, 0);
            tone.process (left + pos, right + pos, n);        // Acid's Bass / Treble
        }
        else
        {
            std::fill (left + pos, left + pos + n, 0.0f);
            std::fill (right + pos, right + pos + n, 0.0f);
        }

        if (b303)
        {
            voice303.render (scratchL, scratchR, n, voiceMod);
            for (int i = 0; i < n; ++i)
            {
                left[pos + i] += scratchL[i];
                right[pos + i] += scratchR[i];
            }
        }
    }
}

void AcidAudioProcessor::playNoteOn (int note, bool accent, bool legato, float level, float filterVelocity, bool glide) noexcept
{
    if (activeEngine == 1)
        voice303.noteOn (note, accent, legato, level, glide);
    else
        voice.noteOn (note, accent, legato, level, filterVelocity, glide);
}

void AcidAudioProcessor::playNoteOff() noexcept
{
    voice.noteOff();
    voice303.noteOff();
}

void AcidAudioProcessor::applyArpEvent (const as::arp::NoteEvent& e) noexcept
{
    using Type = as::arp::NoteEvent::Type;
    // loudness: the note's velocity scaled by the step's Volume (as in ArpSampler)
    const float level = (float) jlimit (0, 127, e.velocity) / 127.0f * (float) jlimit (0, 127, e.volume) / 127.0f;
    // Vel Link follows the step's Velocity lane, whatever the velocity mode (as in ArpSampler)
    const int laneVelocity = jlimit (1, 127, e.stepVelocity);
    const float filterVelocity = (float) laneVelocity / 127.0f;

    switch (e.type)
    {
        case Type::Start:
            // mono: the newest note takes the voice (in Chord mode, the top note)
            playNoteOn (e.note, e.accent, false, level, filterVelocity);
            gui.lastVelocity.store (laneVelocity);
            voiceArpId = e.id;
            for (auto& l : lfos) l.noteTriggered();
            break;

        case Type::Change:
            // a tie: glide to the new pitch over the Glide time, without retriggering
            if (e.id == voiceArpId)
            {
                playNoteOn (e.note, e.accent, true, level, filterVelocity, e.glide);   // Glide slides, Tie jumps
                gui.lastVelocity.store (laneVelocity);
            }
            break;

        case Type::Stop:
            if (e.id == voiceArpId)
            {
                playNoteOff();
                voiceArpId = 0;
            }
            break;

        case Type::LegatoReset:
            break;
    }
}

void AcidAudioProcessor::handleDirectMidi (const MidiMessage& msg) noexcept
{
    // Arp off: the keys play the mono voice. Last-note priority; an overlapping note slides.
    auto remove = [this] (int note)
    {
        for (int i = 0; i < numHeldNotes; ++i)
            if (heldNotes[(size_t) i] == note)
            {
                for (int j = i; j < numHeldNotes - 1; ++j) heldNotes[(size_t) j] = heldNotes[(size_t) j + 1];
                --numHeldNotes;
                return true;
            }
        return false;
    };

    if (msg.isNoteOn())
    {
        const int note = msg.getNoteNumber();
        const bool legato = numHeldNotes > 0;
        remove (note);
        if (numHeldNotes < (int) heldNotes.size()) heldNotes[(size_t) numHeldNotes++] = note;
        const float velocity = (float) msg.getVelocity() / 127.0f;
        playNoteOn (note, msg.getVelocity() >= 100, legato, velocity, velocity);
        gui.lastVelocity.store (msg.getVelocity());
        if (! legato)
            for (auto& l : lfos) l.noteTriggered();
    }
    else if (msg.isNoteOff())
    {
        const int note = msg.getNoteNumber();
        const bool wasTop = numHeldNotes > 0 && heldNotes[(size_t) numHeldNotes - 1] == note;
        if (remove (note) && wasTop)
        {
            if (numHeldNotes > 0)
                playNoteOn (heldNotes[(size_t) numHeldNotes - 1], false, true);
            else
                playNoteOff();
        }
    }
    else if (msg.isAllNotesOff() || msg.isAllSoundOff())
    {
        numHeldNotes = 0;
        playNoteOff();
    }
}

void AcidAudioProcessor::passThrough (const MidiMessage& m, int time) noexcept
{
    // Arp off with MIDI Out on: the keys go straight out
    if (m.isNoteOn())
    {
        thruNotes.set ((size_t) m.getNoteNumber());
        thruChannels[(size_t) m.getNoteNumber()] = (uint8) m.getChannel();
    }
    else if (m.isNoteOff())
    {
        thruNotes.reset ((size_t) m.getNoteNumber());
    }
    midiOutBuffer.addEvent (m, time);
}

void AcidAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;
    const int total = buffer.getNumSamples();
    buffer.clear();
    if (total <= 0 || buffer.getNumChannels() < 2)
        return;

    const auto transport = readTransport();
    updateSettings();

    float* left = buffer.getWritePointer (0);
    float* right = buffer.getWritePointer (1);

    // MIDI Out
    const bool midiOutOn = P.midiOut->load() > 0.5f;
    midiOutBuffer.clear();
    auto send = [this] (const MidiMessage& m, int time) { midiOutBuffer.addEvent (m, time); };
    auto emit = [&] (const MidiMessage& m, int time) { if (midiOutOn) send (m, time); };
    if (midiOutOn && ! midiOutWasOn)
        midiWriter.forgetExpression();
    if (midiOutWasOn && ! midiOutOn)
    {
        midiWriter.allNotesOff (0, send);
        releaseThruNotes (0, send);
    }
    midiOutWasOn = midiOutOn;

    // which synth plays: switching hands over (the old one's note ends and rings out)
    const int engineNow = roundToInt (P.engine->load());
    if (engineNow != activeEngine)
    {
        playNoteOff();
        voiceArpId = 0;
        numHeldNotes = 0;
        activeEngine = engineNow;
    }

    // Sequencer
    const auto settings = readArpSettings();
    if (settings.enabled != arpWasEnabled)
    {
        // hand the voice over between the keys and the sequencer
        playNoteOff();
        numHeldNotes = 0;
        voiceArpId = 0;
        releaseThruNotes (0, send);
    }
    arpWasEnabled = settings.enabled;

    arp.setSettings (settings);
    arp.setPattern (patternExchange.read());
    arpEvents.clear();
    arp.process (midi, total, transport, arpEvents);

    // Render in time order, splitting the block at every sequencer event and MIDI message.
    int rendered = 0, eventIndex = 0;
    auto midiIt = midi.cbegin();
    constexpr int none = std::numeric_limits<int>::max();

    for (;;)
    {
        const int nextEvent = eventIndex < arpEvents.size() ? arpEvents[eventIndex].offset : none;
        const int nextMidi = midiIt != midi.cend() ? (*midiIt).samplePosition : none;
        if (nextEvent == none && nextMidi == none)
            break;

        const int pos = jlimit (rendered, total, jmin (nextEvent, nextMidi));
        renderVoice (left, right, rendered, pos);
        rendered = pos;

        if (nextEvent <= nextMidi)
        {
            const auto& e = arpEvents[eventIndex++];
            applyArpEvent (e);
            midiWriter.apply (e, pos, emit);
        }
        else
        {
            const auto message = (*midiIt).getMessage();
            ++midiIt;

            if (! settings.enabled)
            {
                handleDirectMidi (message);
                if (midiOutOn)
                    passThrough (message, pos);
            }
            else
            {
                if (message.isAllSoundOff() || message.isResetAllControllers())
                {
                    playNoteOff();
                    voiceArpId = 0;
                    midiWriter.allNotesOff (pos, emit);
                }

                // the sequencer owns the notes; controllers, pitch bend and pressure go straight out
                if (midiOutOn && ! message.isNoteOnOrOff() && ! message.isSustainPedalOn() && ! message.isSustainPedalOff()
                    && ! message.isAllNotesOff() && ! message.isAllSoundOff())
                    send (message, pos);
            }
        }
    }
    renderVoice (left, right, rendered, total);

    // --- effects -------------------------------------------------------------------
    distortion.process (left, right, total);     // Saturate / Foldback, then the three-band tube
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

    midi.clear();
    if (! midiOutBuffer.isEmpty())
        midi.addEvents (midiOutBuffer, 0, total, 0);

    // --- GUI state ----------------------------------------------------------------
    gui.running.store (settings.enabled && arp.isRunning());
    gui.hostPlaying.store (hostPlaying);
    gui.currentStep.store (settings.enabled ? arp.getCurrentStep() : -1);
    gui.filterEnv.store (activeEngine == 1 ? voice303.getFilterEnv() : voice.getFilterEnv());
    gui.accentCharge.store (activeEngine == 1 ? voice303.getAccentCharge() : voice.getAccentCharge());
    gui.cutoffHz.store (activeEngine == 1 ? voice303.getLastCutoffHz() : voice.getLastCutoffHz());
    gui.gateOn.store (activeEngine == 1 ? voice303.isGateOn() : voice.isGateOn());
    for (int i = 0; i < 2; ++i) gui.lfoPhase[i].store ((float) lfos[i].getPhase());
    for (int b = 0; b < 3; ++b) gui.bandLevel[b].store (tube.getBandLevel (b));
    publishHeldNotes();
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
    randomiseTubeSettings();
}

void AcidAudioProcessor::randomiseTubeSettings()
{
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

//==============================================================================
// Randomise Patch (header): the sound, within musical ranges, with undo
//==============================================================================
bool AcidAudioProcessor::isPatchParameter (const String& id)
{
    using namespace ParamIDs;
    // the pattern / arp settings, the randomiser and export settings, the level, tuning and
    // the Echo / Tube on-off switches are not part of the patch
    if (id.startsWith ("rnd") || id.startsWith ("export") || id.startsWith ("arp") || id.startsWith ("seq")
        || id.startsWith ("scale"))
        return false;
    for (auto* other : { volume, tune, echoOn, tubeOn, velMode, ParamIDs::bpm, autoPlay, midiOut, engine, distPageOn, distOn })
        if (id == other)
            return false;
    return true;
}

std::vector<std::pair<String, float>> AcidAudioProcessor::capturePatch() const
{
    std::vector<std::pair<String, float>> values;
    for (auto* param : getParameters())
        if (auto* ranged = dynamic_cast<RangedAudioParameter*> (param); ranged != nullptr && isPatchParameter (ranged->getParameterID()))
            values.emplace_back (ranged->getParameterID(), ranged->convertFrom0to1 (ranged->getValue()));
    return values;
}

void AcidAudioProcessor::undoRandomPatch()
{
    if (patchUndo.empty())
        return;
    for (auto& [id, value] : patchUndo.back())
        setParameterValue (id, value);
    patchUndo.pop_back();
}

void AcidAudioProcessor::randomisePatch()
{
    patchUndo.push_back (capturePatch());
    if (patchUndo.size() > 32)
        patchUndo.erase (patchUndo.begin());

    using namespace ParamIDs;
    Random r;
    auto range = [&r] (float lo, float hi) { return std::round (lo + (hi - lo) * r.nextFloat()); };
    auto chance = [&r] (float p) { return r.nextFloat() < p; };
    auto set = [this] (const String& id, float v) { setParameterValue (id, v); };

    // oscillator
    set (wave, chance (0.4f) ? 1.0f : 0.0f);
    set (retro, chance (0.25f) ? 1.0f : 0.0f);
    set (oscALock, chance (0.35f) ? 1.0f : 0.0f);

    // filter: mostly the classic low-pass, the diode ladder more often than not
    const float mode = r.nextFloat();
    set (filterMode, mode < 0.72f ? 0.0f : (mode < 0.86f ? 1.0f : 2.0f));
    set (transistor, chance (0.25f) ? 1.0f : 0.0f);
    set (cutoff, range (22.0f, 58.0f));
    set (cutoffLow, range (10.0f, 50.0f));
    set (resonance, range (45.0f, 90.0f));
    set (drive, range (0.0f, 55.0f));
    set (bass, range (-3.0f, 6.0f));
    set (treble, range (-4.0f, 4.0f));
    set (filterVelLink, chance (0.3f) ? 1.0f : 0.0f);
    set (filterVelAmount, range (20.0f, 80.0f));

    // envelope / accent / glide
    set (envMod, range (30.0f, 85.0f));
    set (decay, range (20.0f, 70.0f));
    set (accent, range (40.0f, 90.0f));
    set (envAttack, range (30.0f, 60.0f));
    set (accDecay, range (15.0f, 55.0f));
    set (accSweep, range (0.0f, 80.0f));
    set (slideTime, range (40.0f, 120.0f));

    // layers
    set (oscBLevel, chance (0.4f) ? range (10.0f, 55.0f) : 0.0f);
    static const float oscBTunes[] = { 0.0f, 0.0f, -12.0f, 12.0f, 7.0f, -5.0f };
    set (oscBTune, oscBTunes[r.nextInt (6)]);
    set (oscBDetune, range (-15.0f, 15.0f));
    set (oscBLock, chance (0.35f) ? 1.0f : 0.0f);
    set (unisonVoices, chance (0.7f) ? 1.0f : range (2.0f, 5.0f));
    set (unisonSpread, range (10.0f, 60.0f));
    set (ampAttack, range (5.0f, 20.0f));
    set (ampDecay, chance (0.6f) ? 100.0f : range (55.0f, 95.0f));
    set (ampRelease, range (10.0f, 35.0f));

    // echo: synced, short to medium
    set (echoSync, 1.0f);
    static const int echoDivs[] = { 5, 7, 8, 10, 11 };     // 1/16, 1/16., 1/8, 1/8., 1/4
    set (echoTime, knobForDivision (echoDivs[r.nextInt (5)], (int) echoDivisions().size()));
    set (echoMix, range (0.0f, 35.0f));
    set (echoFeedback, range (20.0f, 70.0f));
    set (echoLowCut, range (20.0f, 50.0f));
    set (echoHighCut, range (30.0f, 70.0f));
    set (echoWidth, range (40.0f, 100.0f));

    // LFOs: each one on about half the time, mostly on the cutoff
    for (int i = 0; i < 2; ++i)
    {
        const bool on = chance (0.5f);
        set (lfo (i, "Depth"), on ? (chance (0.8f) ? 1.0f : -1.0f) * range (10.0f, 40.0f) : 0.0f);
        set (lfo (i, "Dest"), chance (0.7f) ? (float) DestCutoff : (float) r.nextInt (NumLfoDests));
        set (lfo (i, "Shape"), (float) r.nextInt (lfoShapeNames().size()));
        set (lfo (i, "Sync"), 1.0f);
        static const int lfoDivs[] = { 3, 4, 7, 10, 13 };    // 1/1, 1/2, 1/4, 1/8, 1/16
        set (lfo (i, "Rate"), knobForDivision (lfoDivs[r.nextInt (5)], (int) lfoDivisions().size()));
        set (lfo (i, "Retrig"), (float) r.nextInt (3));
    }

    // the 303 page
    set (b3Wave, chance (0.4f) ? 1.0f : 0.0f);
    set (b3Cutoff, range (20.0f, 70.0f));
    set (b3Resonance, range (40.0f, 95.0f));
    set (b3EnvMod, range (25.0f, 90.0f));
    set (b3Decay, range (10.0f, 70.0f));
    set (b3Accent, range (40.0f, 95.0f));

    // the Saturate / Foldback distortion (its on / off switch stays as it is)
    set (distMode, chance (0.75f) ? 0.0f : 1.0f);
    set (distDrive, range (15.0f, 75.0f));
    set (distAmount, range (30.0f, 100.0f));

    randomiseTubeSettings();
    presets.setCurrentName ("Random");
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
            if (! ranged->getParameterID().startsWith ("rnd") && ! ranged->getParameterID().startsWith ("export"))
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
    state.setProperty (sequencePathID, currentSequence.getFullPathName(), nullptr);
    state.setProperty (seedID, (int64) randomSeed, nullptr);

    state.removeChild (state.getChildWithName (as::arp::PatternModel::treeType), nullptr);
    state.appendChild (patternModel.toValueTree(), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void AcidAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;

    auto state = ValueTree::fromXml (*xml);
    restoringState = true;

    gui.editorWidth.store ((int) state.getProperty ("editorWidth", 0));
    gui.lightTheme.store ((bool) state.getProperty ("lightTheme", false));
    gui.page.store ((int) state.getProperty ("page", 0));
    presets.setCurrentName (state.getProperty ("presetName", "Init").toString());
    {
        const auto path = state.getProperty (sequencePathID).toString();
        currentSequence = File::isAbsolutePath (path) ? File (path) : File();
    }
    randomSeed = (uint32) (int64) state.getProperty (seedID, 1);

    // the pattern: saved by this version, or the first pattern of the original Acid sequencer
    const auto patternTree = state.getChildWithName (as::arp::PatternModel::treeType);
    const auto legacyTree = state.getChildWithName ("Patterns");
    int legacyLength = 0;
    if (patternTree.isValid())
    {
        patternModel.fromValueTree (patternTree);
    }
    else if (legacyTree.isValid() && legacyTree.getNumChildren() > 0)
    {
        const auto converted = patternFromLegacyString (legacyTree.getChild (0).getProperty ("data").toString().toStdString());
        patternModel.set (converted.pattern, false);
        patternModel.clearHistory();
        legacyLength = converted.length;
    }
    state.removeChild (patternTree, nullptr);
    state.removeChild (legacyTree, nullptr);

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

    restoringState = false;
    if (legacyLength > 0)
    {
        setParameterValue (ParamIDs::seqLength, legacyLength > 16 ? 1.0f : 0.0f);
        setParameterValue (ParamIDs::seqEnd, (legacyLength == 16 || legacyLength == 32) ? 0.0f : (float) legacyLength);
    }
    syncPatternUnits();
}

//==============================================================================
AudioProcessorEditor* AcidAudioProcessor::createEditor()
{
    return new AcidAudioProcessorEditor (*this);
}

#if ! ACID_NO_PLUGIN_ENTRY
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AcidAudioProcessor();
}
#endif
