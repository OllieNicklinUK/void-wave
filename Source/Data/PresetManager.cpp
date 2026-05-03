#include "PresetManager.h"
#include "VoidWavePresets.h"

// Factory presets are loaded from Resources/Presets/factory/ at startup.
// Use IMPORT button to load additional preset packs.

// ── importFromFolder ──────────────────────────────────────────────────────────

// Parse a .vwpreset file into a PresetEntry, caching all param values.
static PresetEntry parsePresetFile(const juce::File& f, const juce::String& category, bool isFactory)
{
    PresetEntry e;
    e.name      = f.getFileNameWithoutExtension().replace("_", " ");
    e.category  = category;
    e.isFactory = isFactory;
    e.file      = f;

    // Parse XML and cache every attribute as an actual (un-normalised) value.
    // loadPreset() will call setParam(key, value) which normalises on the way in.
    auto xml = juce::XmlDocument::parse(f);
    if (xml != nullptr)
    {
        // Read metadata attributes (prefixed with _)
        e.sub  = xml->getStringAttribute("_sub",  "");
        e.name = xml->getStringAttribute("_name", e.name);  // override filename if present
        for (int i = 0; i < xml->getNumAttributes(); ++i)
        {
            auto key = xml->getAttributeName(i);
            if (!key.startsWith("_"))   // skip metadata
                e.params.push_back({ key, static_cast<float>(xml->getDoubleAttribute(key)) });
        }
    }
    return e;
}

int PresetManager::importFromFolder(const juce::File& root)
{
    int added = 0;

    juce::Array<juce::File> items;
    root.findChildFiles(items, juce::File::findDirectories, false);
    items.sort();

    if (items.isEmpty())
    {
        // Flat folder — category = folder name
        juce::String cat = root.getFileName();
        juce::Array<juce::File> files;
        root.findChildFiles(files, juce::File::findFiles, false, "*.vwpreset");
        files.sort();
        for (const auto& f : files)
        {
            presets.push_back(parsePresetFile(f, cat, false));
            ++added;
        }
    }
    else
    {
        for (const auto& catDir : items)
        {
            juce::String cat = catDir.getFileName();
            juce::Array<juce::File> files;
            catDir.findChildFiles(files, juce::File::findFiles, false, "*.vwpreset");
            files.sort();
            for (const auto& f : files)
            {
                presets.push_back(parsePresetFile(f, cat, false));
                ++added;
            }
        }
    }

    if (added > 0) currentIndex = 0;
    return added;
}

// ── PresetManager ─────────────────────────────────────────────────────────────

PresetManager::PresetManager(juce::AudioProcessorValueTreeState& a) : apvts(a)
{
    refresh();
}

juce::File PresetManager::getUserPresetFolder()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
               .getChildFile("Void Audio/Void Wave/Presets");
}

void PresetManager::refresh()
{
    presets.clear();
    loadFactoryPresets();
    scanUserFolder();
}

void PresetManager::loadFactoryPresets()
{
    // Try the source tree first (dev builds — exe lives inside the project dir).
    juce::File dir = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    for (int i = 0; i < 12; ++i)
    {
        dir = dir.getParentDirectory();
        juce::File factory = dir.getChildFile("Resources/Presets/factory");
        if (factory.isDirectory())
        {
            importFromFolder(factory);
            for (auto& p : presets) p.isFactory = true;
            return;
        }
    }

    // Source tree not found — use presets embedded at compile time.
    loadEmbeddedPresets();
}

void PresetManager::loadEmbeddedPresets()
{
    // Iterate every compiled-in resource, pick out .vwpreset files.
    for (int i = 0; i < VWPresets::namedResourceListSize; ++i)
    {
        const char* resName = VWPresets::namedResourceList[i];
        juce::String rn(resName);
        if (!rn.endsWith("_vwpreset")) continue;

        int sz = 0;
        const char* data = VWPresets::getNamedResource(resName, sz);
        if (!data || sz == 0) continue;

        auto xml = juce::XmlDocument::parse(juce::String::fromUTF8(data, sz));
        if (!xml) continue;

        PresetEntry e;
        e.name     = xml->getStringAttribute("_name",     rn.replace("_vwpreset","").replace("_"," "));
        e.category = xml->getStringAttribute("_category", "FACTORY");
        e.sub      = xml->getStringAttribute("_sub",      "");
        e.isFactory = true;

        for (int j = 0; j < xml->getNumAttributes(); ++j)
        {
            auto key = xml->getAttributeName(j);
            if (!key.startsWith("_"))
                e.params.push_back({ key, static_cast<float>(xml->getDoubleAttribute(key)) });
        }

        if (e.name.isNotEmpty())
            presets.push_back(std::move(e));
    }

    // Sort by category then name so the browser looks tidy.
    std::sort(presets.begin(), presets.end(), [](const PresetEntry& a, const PresetEntry& b)
    {
        return a.category != b.category ? a.category < b.category : a.name < b.name;
    });

    if (!presets.empty()) currentIndex = 0;
}

void PresetManager::scanUserFolder()
{
    juce::File folder = getUserPresetFolder();
    if (!folder.isDirectory()) return;

    // Explicit two-level walk: category dirs → .vwpreset files
    juce::Array<juce::File> catDirs;
    folder.findChildFiles(catDirs, juce::File::findDirectories, false);
    catDirs.sort();

    for (const auto& catDir : catDirs)
    {
        juce::String category = catDir.getFileName();
        juce::Array<juce::File> presetFiles;
        catDir.findChildFiles(presetFiles, juce::File::findFiles, false, "*.vwpreset");
        presetFiles.sort();

        for (const auto& f : presetFiles)
        {
            PresetEntry e;
            e.name      = f.getFileNameWithoutExtension().replace("_", " ");
            e.category  = category;
            e.isFactory = false;
            e.file      = f;
            presets.push_back(std::move(e));
        }
    }
}

void PresetManager::setParam(const juce::String& id, float value) const
{
    if (auto* param = apvts.getParameter(id))
        param->setValueNotifyingHost(param->convertTo0to1(value));
}

bool PresetManager::loadPreset(int index)
{
    if (index < 0 || index >= (int)presets.size()) return false;
    currentIndex = index;

    const auto& entry = presets[(size_t)index];

    // Use cached params (parsed at import time) — avoids disk I/O and
    // normalisation mismatch. setParam() converts actual→normalised correctly.
    if (!entry.params.empty())
    {
        for (const auto& kv : entry.params)
            setParam(kv.first, kv.second);
        return true;
    }

    // Fallback: re-parse from file (handles presets added without caching)
    if (!entry.file.existsAsFile()) return false;
    auto xml = juce::XmlDocument::parse(entry.file);
    if (!xml) return false;
    for (int i = 0; i < xml->getNumAttributes(); ++i)
        setParam(xml->getAttributeName(i),
                 static_cast<float>(xml->getDoubleAttribute(xml->getAttributeName(i))));
    return true;
}

bool PresetManager::savePreset(const juce::String& name, const juce::String& category)
{
    juce::File folder = getUserPresetFolder().getChildFile(category);
    if (!folder.createDirectory()) return false;
    juce::File file = folder.getChildFile(name + ".vwpreset");
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    return xml && xml->writeTo(file);
}

void PresetManager::nextPreset()
{
    if (presets.empty()) return;
    loadPreset((currentIndex + 1) % (int)presets.size());
}

void PresetManager::previousPreset()
{
    if (presets.empty()) return;
    loadPreset((currentIndex - 1 + (int)presets.size()) % (int)presets.size());
}
