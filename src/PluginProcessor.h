#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"
#include "dsp/Voice.h"
#include "dsp/Lfo.h"
#include "dsp/ToneStack.h"
#include "dsp/MultibandTube.h"
#include "dsp/Echo.h"
#include "seq/Sequencer.h"
#include "seq/Pattern.h"
#include "seq/PatternTools.h"
#include "presets/PresetManager.h"

//==============================================================================
/** Lock-free hand-over of the pattern bank from the message thread to the audio
    thread (triple buffer: neither side ever waits). */
class PatternExchange
{
public:
    PatternExchange()
    {
        for (auto& b : banks)
            for (auto& p : b)
                p = acid::makeDefaultPattern();
    }

    /** Message thread. */
    void publish (const acid::PatternBank& bank)
    {
        banks[(size_t) back] = bank;
        back = middle.exchange (back | kDirty) & kIndexMask;
    }

    /** Audio thread. */
    const acid::PatternBank& read() noexcept
    {
        if (middle.load() & kDirty)
            front = middle.exchange (front) & kIndexMask;
        return banks[(size_t) front];
    }

private:
    static constexpr int kDirty = 4, kIndexMask = 3;
    std::array<acid::PatternBank, 3> banks;
    std::atomic<int> middle { 1 };
    int front = 0, back = 2;
};

//==============================================================================
class AcidAudioProcessor : public juce::AudioProcessor,
                           private juce::AudioProcessorValueTreeState::Listener
{
public:
    AcidAudioProcessor();
    ~AcidAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void reset() override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                 { return true; }

    const juce::String getName() const override     { return JucePlugin_Name; }
    bool acceptsMidi() const override               { return true; }
    bool producesMidi() const override              { return true; }
    bool isMidiEffect() const override              { return false; }
    double getTailLengthSeconds() const override    { return 4.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

    //==============================================================================
    // Patterns (message thread)
    int getCurrentSlot() const;
    const acid::Pattern& getPattern (int slot) const    { return bank[(size_t) juce::jlimit (0, acid::kNumPatterns - 1, slot)]; }
    const acid::PatternBank& getBank() const            { return bank; }

    /** Edits a pattern slot (with undo) and hands the change to the audio thread. */
    void editPattern (int slot, const std::function<void (acid::Pattern&)>& edit, bool undoable = true);
    void setBank (const acid::PatternBank& newBank);    // presets: no undo

    void undoPattern();
    void redoPattern();
    bool canUndoPattern() const { return patternUndo.canUndo(); }
    bool canRedoPattern() const { return patternUndo.canRedo(); }
    void copyPattern();
    void pastePattern();
    bool hasClipboard() const   { return clipboardValid; }
    void randomisePattern (acid::PatternTools::RandomMode);
    void mutatePattern();
    void rotatePattern (int direction);
    void clearPattern();

    int randomRoot = 9;         // A
    int randomScale = 1;        // Minor

    /** Changes every time the patterns change (the editor polls it to repaint). */
    std::atomic<int> patternVersion { 0 };

    //==============================================================================
    // Transport / misc (message thread)
    void setInternalPlay (bool shouldPlay)  { internalPlayRequest.store (shouldPlay); }
    bool isInternalPlayOn() const           { return internalPlayRequest.load(); }
    std::atomic<bool> midiOutEnabled { true };

    /** Sets a parameter as one complete host gesture. */
    void setParameterValue (const juce::String& id, float value);
    float getParameterValue (const juce::String& id) const;

    // Tube page: Generate / Undo / Init / Redo
    void generateTube();
    void initTube();
    void undoTube();
    void redoTube();
    bool canUndoTube() const { return ! tubeUndo.empty(); }
    bool canRedoTube() const { return ! tubeRedo.empty(); }

    /** Header Clear: every sound parameter back to its default (patterns stay). */
    void initSound();

    acid::PresetManager presets;

    //==============================================================================
    /** Read-only state for the editor (written on the audio thread). */
    struct GuiState
    {
        std::atomic<int>   currentStep { -1 };
        std::atomic<bool>  running { false };
        std::atomic<bool>  hostPlaying { false };
        std::atomic<float> bpm { 120.0f };
        std::atomic<float> filterEnv { 0.0f };
        std::atomic<float> accentCharge { 0.0f };
        std::atomic<float> cutoffHz { 1000.0f };
        std::atomic<float> lfoPhase[2] { 0.0f, 0.0f };
        std::atomic<float> bandLevel[3] { 0.0f, 0.0f, 0.0f };
        std::atomic<bool>  gateOn { false };
        std::atomic<int>   editorWidth { 0 };
        std::atomic<bool>  lightTheme { false };
        std::atomic<int>   page { 0 };
    };
    GuiState gui;

private:
    void parameterChanged (const juce::String& id, float newValue) override;
    void updateSettings() noexcept;
    void processChunk (float* left, float* right, int numSamples, int chunkStart,
                       const juce::MidiBuffer& midiIn, juce::MidiBuffer& midiOut,
                       const acid::Pattern& pattern) noexcept;
    void applyVoiceEvent (const acid::SeqEvent&, bool fromSequencer, juce::MidiBuffer& midiOut, int sampleInBlock) noexcept;
    void computeModulation (int start, int end) noexcept;
    double ppqAt (int sampleInBlock) const noexcept;
    void pushTubeUndo();
    std::vector<float> captureTube() const;
    void restoreTube (const std::vector<float>&);
    static juce::StringArray tubeParameterIds();

    // DSP
    acid::Voice voice;
    acid::Lfo lfos[2];
    acid::ToneStack tone;
    acid::MultibandTube tube;
    acid::Echo echo;
    acid::Sequencer sequencer;
    juce::SmoothedValue<float> masterGain;

    static constexpr int kChunk = 512;
    float lfoBuf[2][kChunk] {};
    float modCutoff[kChunk] {}, modPitch[kChunk] {}, modWave[kChunk] {}, modReso[kChunk] {}, modVolume[kChunk] {};
    acid::VoiceMod voiceMod;
    acid::SeqEvent seqEvents[acid::Sequencer::kMaxEvents];

    // block state
    double sampleRate = 44100.0;
    double blockPpq = 0.0, ppqPerSample = 0.0, internalPpq = 0.0, internalBlockStart = 0.0;
    double bpm = 120.0;
    bool hostPlaying = false, internalRunning = false, keyClockRunning = false;
    int chunkOffset = 0;

    // MIDI note stacks (direct play / key mode)
    std::array<int, 16> heldNotes {}, heldKeys {};
    int numHeldNotes = 0, numHeldKeys = 0;
    int midiOutNote = -1;
    bool directVoiceActive = false;

    std::atomic<bool> internalPlayRequest { false };
    bool internalPlayState = false;

    // cached parameter pointers (audio thread reads these)
    struct ParamPointers
    {
        std::atomic<float> *tune, *volume, *echoOn, *tubeOn, *wave, *retro, *cutoffLow, *cutoff, *resonance,
            *transistor, *bass, *treble, *drive, *envMod, *decay, *accent, *gate, *envAttack, *accDecay, *accSweep,
            *seqOn, *keyMode, *seqRate, *swing, *slideTime, *direction, *transpose, *pattern,
            *oscBLevel, *oscBTune, *oscBDetune, *unisonVoices, *unisonSpread, *unisonFree,
            *ampAttack, *ampDecay, *ampRelease, *echoSync, *echoTime, *echoMix, *echoFeedback, *echoLowCut,
            *echoHighCut, *echoWidth, *tubeEdges[4], *tubeResonator, *tubeCharacter, *tubeInput, *tubeOutput, *tubeMix;
        std::atomic<float> *bandDrive[3], *bandGain[3], *bandSolo[3];
        std::atomic<float> *lfoRate[2], *lfoDepth[2], *lfoPhase[2], *lfoShape[2], *lfoSync[2], *lfoMirror[2], *lfoRetrig[2], *lfoDest[2];
    } P {};
    float lfoDepthNow[2] {};
    int lfoDestNow[2] {};

    // patterns
    acid::PatternBank bank;
    PatternExchange exchange;
    acid::PatternUndoStack patternUndo;
    acid::Pattern clipboard;
    bool clipboardValid = false;
    uint32_t randomSeed = 1;

    // tube undo
    std::vector<std::vector<float>> tubeUndo, tubeRedo;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AcidAudioProcessor)
};
