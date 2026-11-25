#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <bitset>
#include <optional>
#include "Parameters.h"
#include "dsp/Voice.h"
#include "dsp/Lfo.h"
#include "dsp/ToneStack.h"
#include "dsp/MultibandTube.h"
#include "dsp/Echo.h"
#include "arp/ArpEngine.h"
#include "arp/PatternModel.h"
#include "arp/Randomiser.h"
#include "arp/MidiNoteWriter.h"
#include "arp/export/MidiRenderer.h"
#include "arp/import/PatternImport.h"
#include "arp/browser/SampleLibrary.h"
#include "ui/arp/BrowserPanel.h"
#include "presets/PresetManager.h"

//==============================================================================
class AcidAudioProcessor : public juce::AudioProcessor,
                           private juce::AudioProcessorValueTreeState::Listener,
                           private juce::AsyncUpdater
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
    bool producesMidi() const override              { return true; }    // MIDI Out
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
    // Sequencer (the ArpSampler arpeggiator + step sequencer), message thread
    as::arp::PatternModel& getPatternModel() noexcept   { return patternModel; }

    /** Replaces the pattern (undoable) and sets Length / End for 'length' steps (presets). */
    void setPattern (const as::arp::Pattern&, int length);
    /** Restores a saved pattern tree (user presets). */
    void restorePattern (const juce::ValueTree&);

    void randomisePattern (std::optional<juce::uint32> seed = std::nullopt);
    as::arp::RandomiseSettings readRandomiseSettings() const;
    int quantiseEditedTranspose (int transpose) const    { return transpose; }   // scale steps are always in key
    as::arp::Pattern makeEmptyPattern() const;

    struct ImportChoices
    {
        int startBar = 1;           // 1-based bar of the file the steps start from
        bool topNote = true;        // chords: keep the top note
    };
    ImportChoices& getImportChoices() noexcept          { return importChoices; }
    as::import::ImportOptions makeImportOptions() const;
    int applyImport (const std::vector<as::import::ImportedNote>&, bool lockToScale = false);

    SampleLibrary& getSequenceLibrary() noexcept        { return *sequenceLibrary; }
    BrowserPanel::State& getSequenceBrowserState() noexcept { return sequenceBrowserState; }
    bool isSequenceBrowserOpen() const noexcept         { return sequenceBrowserOpen; }
    void setSequenceBrowserOpen (bool open) noexcept    { sequenceBrowserOpen = open; }
    juce::File getCurrentSequenceFile() const           { return currentSequence; }
    void setCurrentSequenceFile (const juce::File& f)   { currentSequence = f; }

    as::exporter::ExportRequest buildExportRequest() const;
    juce::File exportMidiToTempFile() const;
    std::vector<as::arp::HeldNote> getLastHeldNotes() const;

    int getCurrentStep() const noexcept                 { return gui.currentStep.load(); }
    float getTempo() const noexcept                     { return gui.bpm.load(); }
    bool isTempoFromHost() const noexcept               { return gui.hostTempo.load(); }

    //==============================================================================
    void setParameterValue (const juce::String& id, float value);
    float getParameterValue (const juce::String& id) const;

    // Tube page: Generate / Undo / Init / Redo
    void generateTube();
    void initTube();
    void undoTube();
    void redoTube();
    bool canUndoTube() const { return ! tubeUndo.empty(); }
    bool canRedoTube() const { return ! tubeRedo.empty(); }

    /** Header Clear: every sound parameter back to its default (the pattern stays). */
    void initSound();

    /** Header Randomise: a new patch within musical ranges (the pattern and arp settings stay). */
    void randomisePatch();
    void undoRandomPatch();
    bool canUndoRandomPatch() const { return ! patchUndo.empty(); }
    static bool isPatchParameter (const juce::String& id);

    acid::PresetManager presets;

    //==============================================================================
    /** Read-only state for the editor (written on the audio thread). */
    struct GuiState
    {
        std::atomic<int>   currentStep { -1 };
        std::atomic<bool>  running { false };
        std::atomic<bool>  hostPlaying { false };
        std::atomic<bool>  hostTempo { false };
        std::atomic<float> bpm { 120.0f };
        std::atomic<float> filterEnv { 0.0f };
        std::atomic<float> accentCharge { 0.0f };
        std::atomic<float> cutoffHz { 1000.0f };
        std::atomic<float> lfoPhase[2] { 0.0f, 0.0f };
        std::atomic<float> bandLevel[3] { 0.0f, 0.0f, 0.0f };
        std::atomic<bool>  gateOn { false };
        std::atomic<int>   lastVelocity { 127 };   // what Vel Link used for the last note
        std::atomic<int>   editorWidth { 0 };
        std::atomic<bool>  lightTheme { false };
        std::atomic<int>   page { 0 };
    };
    GuiState gui;

private:
    void parameterChanged (const juce::String& id, float newValue) override;
    void handleAsyncUpdate() override;
    void syncPatternUnits();
    void updateSettings() noexcept;
    as::arp::ArpSettings readArpSettings() const noexcept;
    as::arp::TransportInfo readTransport() noexcept;
    void renderVoice (float* left, float* right, int from, int to) noexcept;
    void computeModulation (int sampleInBlock, int numSamples) noexcept;
    void applyArpEvent (const as::arp::NoteEvent&) noexcept;
    void handleDirectMidi (const juce::MidiMessage&) noexcept;
    void passThrough (const juce::MidiMessage&, int time) noexcept;
    void publishHeldNotes() noexcept;
    void pushTubeUndo();
    void randomiseTubeSettings();
    std::vector<std::pair<juce::String, float>> capturePatch() const;
    std::vector<std::vector<std::pair<juce::String, float>>> patchUndo;
    std::vector<float> captureTube() const;
    void restoreTube (const std::vector<float>&);
    static juce::StringArray tubeParameterIds();

    template <typename Send>
    void releaseThruNotes (int time, Send&& send) noexcept
    {
        for (int n = 0; n < 128; ++n)
            if (thruNotes[(size_t) n])
                send (juce::MidiMessage::noteOff (juce::jmax (1, (int) thruChannels[(size_t) n]), n), time);
        thruNotes.reset();
    }

    // DSP
    acid::Voice voice;
    acid::Lfo lfos[2];
    acid::ToneStack tone;
    acid::MultibandTube tube;
    acid::Echo echo;
    juce::SmoothedValue<float> masterGain;

    static constexpr int kChunk = 512;
    float lfoBuf[2][kChunk] {};
    float modCutoff[kChunk] {}, modPitch[kChunk] {}, modWave[kChunk] {}, modReso[kChunk] {}, modVolume[kChunk] {};
    acid::VoiceMod voiceMod;

    // Sequencer
    as::arp::PatternExchange patternExchange;
    as::arp::PatternModel patternModel { patternExchange };
    as::arp::ArpEngine arp;
    as::arp::EventList arpEvents;
    bool arpWasEnabled = false;
    int voiceArpId = 0;            // the arp note the (mono) voice is playing (0 = none)

    // MIDI Out
    as::arp::MidiNoteWriter midiWriter;
    juce::MidiBuffer midiOutBuffer;
    std::bitset<128> thruNotes;
    std::array<juce::uint8, 128> thruChannels {};
    bool midiOutWasOn = false;

    // block state
    double sampleRate = 44100.0;
    double blockPpq = 0.0, ppqPerSample = 0.0, bpm = 120.0;
    bool hostPlaying = false;

    // direct play (arp off): last-note priority, overlapping notes slide
    std::array<int, 16> heldNotes {};
    int numHeldNotes = 0;

    struct ParamPointers
    {
        std::atomic<float> *tune, *volume, *echoOn, *tubeOn, *wave, *retro, *cutoffLow, *cutoff, *resonance,
            *transistor, *filterVelLink, *filterVelAmount, *filterMode, *oscALock, *oscBLock, *bass, *treble, *drive, *envMod, *decay, *accent, *envAttack, *accDecay, *accSweep, *slideTime,
            *arpOn, *arpMode, *arpRate, *arpRateType, *arpGate, *arpSwing, *arpOctave, *velMode, *bpm, *autoPlay,
            *midiOut, *seqLength, *seqEnd, *scaleKey, *scale, *scaleLock,
            *oscBLevel, *oscBTune, *oscBDetune, *unisonVoices, *unisonSpread, *unisonFree,
            *ampAttack, *ampDecay, *ampRelease, *echoSync, *echoTime, *echoMix, *echoFeedback, *echoLowCut,
            *echoHighCut, *echoWidth, *tubeEdges[4], *tubeResonator, *tubeCharacter, *tubeInput, *tubeOutput, *tubeMix;
        std::atomic<float> *bandDrive[3], *bandGain[3], *bandSolo[3];
        std::atomic<float> *lfoRate[2], *lfoDepth[2], *lfoPhase[2], *lfoShape[2], *lfoSync[2], *lfoMirror[2], *lfoRetrig[2], *lfoDest[2];
    } P {};
    float lfoDepthNow[2] {};
    int lfoDestNow[2] {};

    // Sequencer extras (message thread)
    ImportChoices importChoices;
    juce::SharedResourcePointer<SequenceLibrary> sequenceLibrary;
    BrowserPanel::State sequenceBrowserState;
    bool sequenceBrowserOpen = false;
    juce::File currentSequence;
    juce::uint32 randomSeed = 1;
    std::atomic<bool> restoringState { false };

    // last held keys, written by the audio thread with a seqlock (for Drag MIDI's Auto root)
    std::atomic<juce::uint32> heldVersion { 0 };
    std::atomic<int> heldCount { 0 };
    std::array<std::atomic<int>, as::arp::kMaxHeld> heldNotesPublished {}, heldVelocities {};
    std::array<as::arp::HeldNote, as::arp::kMaxHeld> lastPublishedHeld {};
    int lastPublishedCount = 0;

    // tube undo
    std::vector<std::vector<float>> tubeUndo, tubeRedo;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AcidAudioProcessor)
};
