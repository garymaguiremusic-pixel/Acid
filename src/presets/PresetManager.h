#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class AcidAudioProcessor;

namespace acid
{
/** Factory presets (built in) and user presets (JSON files in Documents/<Plugin>/Presets).
    A preset holds every parameter, the 16-pattern bank, a name and a bank tag. */
class PresetManager
{
public:
    explicit PresetManager (AcidAudioProcessor&);

    struct Entry { juce::String name, bank; bool factory; int factoryIndex; juce::File file; };

    /** Factory presets first, then the user folder (rescanned on every call). */
    std::vector<Entry> list() const;

    int getNumFactory() const;
    juce::String getFactoryName (int index) const;
    int getCurrentFactoryIndex() const          { return currentFactory; }

    void loadFactory (int index);
    bool loadFile (const juce::File&);
    bool saveFile (const juce::File&, const juce::String& name);
    void loadEntry (const Entry&);

    /** Steps through the combined list (wraps). */
    void step (int delta);

    juce::String getCurrentName() const         { return currentName; }
    juce::String getCurrentBank() const         { return currentBank; }
    void setCurrentName (const juce::String& n) { currentName = n; currentBank = {}; currentFactory = -1; currentFile = juce::File(); ++version; }

    static juce::File userFolder();

    /** Bumped on every load / save (the editor polls it). */
    std::atomic<int> version { 0 };

private:
    void resetAllParameters();
    static bool keepsItsValue (const juce::String& id);
    void applyParameter (const juce::String& id, float value);

    AcidAudioProcessor& processor;
    juce::String currentName { "Init" }, currentBank;
    int currentFactory = -1;
    juce::File currentFile;
};
} // namespace acid
