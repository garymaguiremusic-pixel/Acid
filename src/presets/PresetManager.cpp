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

bool PresetManager::keepsItsValue (const String& id)
{
    // like ArpSampler: presets keep your randomiser / export settings, tempo and routing
    return id.startsWith ("rnd") || id.startsWith ("export") || id == ParamIDs::bpm
        || id == ParamIDs::midiOut || id == ParamIDs::autoPlay;
}

void PresetManager::resetAllParameters()
{
    for (auto* param : processor.getParameters())
        if (auto* ranged = dynamic_cast<RangedAudioParameter*> (param); ranged != nullptr && ! keepsItsValue (ranged->getParameterID()))
        {
            ranged->beginChangeGesture();
            ranged->setValueNotifyingHost (ranged->getDefaultValue());
            ranged->endChangeGesture();
        }
}

void PresetManager::applyParameter (const String& id, float value)
{
    if (processor.apvts.getParameter (id) != nullptr && ! keepsItsValue (id))
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
        applyParameter (id, isHzKnob (id) ? legacyKnobToHz (id, value) : value);   // factory values are knob positions

    const auto converted = patternFromNotation (preset.pattern);
    processor.setPattern (converted.pattern, converted.length);

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
    const int fileVersion = (int) json.getProperty ("version", 0);
    if (auto* params = json.getProperty ("params", var()).getDynamicObject())
        for (auto& prop : params->getProperties())
        {
            const auto id = prop.name.toString();
            const float v = (float) prop.value;
            applyParameter (id, fileVersion < 3 && isHzKnob (id) ? legacyKnobToHz (id, v) : v);   // older files: knob positions
        }

    if (auto xml = parseXML (json.getProperty ("pattern", "").toString()))
        processor.restorePattern (ValueTree::fromXml (*xml));
    else if (auto* arr = json.getProperty ("patterns", var()).getArray(); arr != nullptr && ! arr->isEmpty())
    {
        const auto converted = patternFromLegacyString ((*arr)[0].toString().toStdString());   // first Acid version
        processor.setPattern (converted.pattern, converted.length);
    }

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

    root->setProperty ("pattern", processor.getPatternModel().toValueTree().toXmlString());

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
