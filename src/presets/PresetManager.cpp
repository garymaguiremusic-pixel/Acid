#include "PresetManager.h"
#include "FactoryPresets.h"
#include "../PluginProcessor.h"

namespace acid
{
using namespace juce;

PresetManager::PresetManager (AcidAudioProcessor& p) : processor (p) {}

File PresetManager::userFolder()
{
    return File::getSpecialLocation (File::userDocumentsDirectory)
        .getChildFile (ACID_PRODUCT_NAME).getChildFile ("Presets");
}

int PresetManager::getNumFactory() const              { return (int) factoryPresets().size(); }
String PresetManager::getFactoryName (int i) const
{
    const auto& f = factoryPresets();
    return isPositiveAndBelow (i, (int) f.size()) ? String (f[(size_t) i].name) : String();
}

std::vector<PresetManager::Entry> PresetManager::list() const
{
    std::vector<Entry> entries;
    const auto& f = factoryPresets();
    for (int i = 0; i < (int) f.size(); ++i)
        entries.push_back ({ f[(size_t) i].name, f[(size_t) i].bank, true, i, {} });

    auto files = userFolder().findChildFiles (File::findFiles, false, "*.json");
    files.sort();
    for (auto& file : files)
    {
        String bank = "USER";
        if (auto json = JSON::parse (file); json.isObject())
            bank = json.getProperty ("bank", "USER").toString();
        entries.push_back ({ file.getFileNameWithoutExtension(), bank, false, -1, file });
    }
    return entries;
}

void PresetManager::resetAllParameters()
{
    for (auto* param : processor.getParameters())
        if (auto* ranged = dynamic_cast<RangedAudioParameter*> (param))
        {
            ranged->beginChangeGesture();
            ranged->setValueNotifyingHost (ranged->getDefaultValue());
            ranged->endChangeGesture();
        }
}

void PresetManager::applyParameter (const String& id, float value)
{
    if (processor.apvts.getParameter (id) != nullptr)
        processor.setParameterValue (id, value);
}

void PresetManager::loadFactory (int index)
{
    const auto& f = factoryPresets();
    if (! isPositiveAndBelow (index, (int) f.size()))
        return;
    const auto& preset = f[(size_t) index];

    resetAllParameters();
    for (auto& [id, value] : preset.params)
        applyParameter (id, value);

    PatternBank bank;
    for (int i = 0; i < kNumPatterns; ++i)
        bank[(size_t) i] = i < (int) preset.patterns.size() ? patternFromNotation (preset.patterns[(size_t) i])
                                                            : patternFromNotation (preset.patterns[0]);
    processor.setBank (bank);

    currentName = preset.name;
    currentBank = preset.bank;
    currentFactory = index;
    currentFile = File();
    ++version;
}

bool PresetManager::loadFile (const File& file)
{
    auto json = JSON::parse (file);
    if (! json.isObject())
        return false;

    resetAllParameters();
    if (auto* params = json.getProperty ("params", var()).getDynamicObject())
        for (auto& prop : params->getProperties())
            applyParameter (prop.name.toString(), (float) prop.value);

    PatternBank bank;
    for (auto& p : bank) p = makeDefaultPattern();
    if (auto* arr = json.getProperty ("patterns", var()).getArray())
        for (int i = 0; i < arr->size() && i < kNumPatterns; ++i)
            patternFromString ((*arr)[i].toString().toStdString(), bank[(size_t) i]);
    processor.setBank (bank);

    currentName = json.getProperty ("name", file.getFileNameWithoutExtension()).toString();
    currentBank = json.getProperty ("bank", "USER").toString();
    currentFactory = -1;
    currentFile = file;
    ++version;
    return true;
}

bool PresetManager::saveFile (const File& file, const String& name)
{
    auto root = std::make_unique<DynamicObject>();
    root->setProperty ("format", String (ACID_PRODUCT_NAME) + " preset");
    root->setProperty ("version", kStateVersion);
    root->setProperty ("name", name);
    root->setProperty ("bank", "USER");

    auto params = std::make_unique<DynamicObject>();
    for (auto* param : processor.getParameters())
        if (auto* ranged = dynamic_cast<RangedAudioParameter*> (param))
            params->setProperty (ranged->getParameterID(), ranged->convertFrom0to1 (ranged->getValue()));
    root->setProperty ("params", var (params.release()));

    Array<var> patterns;
    for (auto& p : processor.getBank())
        patterns.add (String (patternToString (p)));
    root->setProperty ("patterns", patterns);

    file.getParentDirectory().createDirectory();
    if (! file.replaceWithText (JSON::toString (var (root.release()))))
        return false;

    currentName = name;
    currentBank = "USER";
    currentFactory = -1;
    currentFile = file;
    ++version;
    return true;
}

void PresetManager::loadEntry (const Entry& e)
{
    if (e.factory) loadFactory (e.factoryIndex);
    else           loadFile (e.file);
}

void PresetManager::step (int delta)
{
    const auto entries = list();
    if (entries.empty())
        return;

    int current = -1;
    for (int i = 0; i < (int) entries.size(); ++i)
        if ((entries[(size_t) i].factory && entries[(size_t) i].factoryIndex == currentFactory && currentFactory >= 0)
            || (! entries[(size_t) i].factory && entries[(size_t) i].file == currentFile && currentFile != File()))
            current = i;

    const int n = (int) entries.size();
    const int next = current < 0 ? (delta > 0 ? 0 : n - 1) : ((current + delta) % n + n) % n;
    loadEntry (entries[(size_t) next]);
}
} // namespace acid
