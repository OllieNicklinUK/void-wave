#pragma once
#include <JuceHeader.h>

struct PresetEntry
{
    juce::String name;
    juce::String category;
    juce::String sub;        // sub-category (e.g. "ACID", "REESE", "LUSH")
    bool         isFactory = true;
    juce::File   file;

    std::vector<std::pair<juce::String, float>> params;
};

class PresetManager
{
public:
    explicit PresetManager(juce::AudioProcessorValueTreeState& apvts);

    void refresh();

    int                getNumPresets()    const { return (int)presets.size(); }
    const PresetEntry& getPreset(int i)   const { return presets[(size_t)i]; }
    int                getCurrentIndex()  const { return currentIndex; }

    bool loadPreset(int index);
    bool savePreset(const juce::String& name, const juce::String& category);
    bool savePresetToDisk(const juce::String& name, const juce::File& file);

    void nextPreset();
    void previousPreset();

    // Import all .vwpreset files from a folder tree into the in-memory list.
    // Category is taken from the immediate parent folder name.
    // Returns number of presets added.
    int importFromFolder(const juce::File& folder);

    static juce::File getUserPresetFolder();

private:
    juce::AudioProcessorValueTreeState& apvts;
    std::vector<PresetEntry>            presets;
    int                                 currentIndex = 0;

    void loadFactoryPresets();
    void loadEmbeddedPresets();   // fallback when source tree not present
    void scanUserFolder();

    // Sets a parameter by its string ID to an un-normalised value.
    void setParam(const juce::String& id, float value) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetManager)
};
