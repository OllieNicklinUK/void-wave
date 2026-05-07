#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"

// Three-column preset browser panel (CATEGORY | PATCH | INFO).
// Shown as an overlay when the user clicks the preset display button.
class PresetBrowser : public juce::Component,
                      public juce::KeyListener
{
public:
    explicit PresetBrowser(VoidWaveAudioProcessor& p);

    // Rebuild lists from PresetManager (call after import / save)
    void refresh();

    // Callbacks set by PluginEditor
    std::function<void(int pmIndex)> onPresetSelected;
    std::function<void()>            onClose;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseExit(const juce::MouseEvent&) override;
    bool keyPressed(const juce::KeyPress&, juce::Component*) override;

private:
    VoidWaveAudioProcessor& processor;

    // Data
    juce::StringArray      categories;
    std::vector<int>       patchIndices;   // PresetManager indices in current cat
    int selCat   = 0;
    int selPatch = -1;
    int hovCat   = -1;
    int hovPatch = -1;
    int catScroll   = 0;
    int patchScroll = 0;

    // Layout (set in resized)
    juce::Rectangle<int> panel;        // the visible browser rectangle
    juce::Rectangle<int> catCol;
    juce::Rectangle<int> patchCol;
    juce::Rectangle<int> infoCol;
    juce::Rectangle<int> closeBtn;

    juce::TextButton newPresetBtn { "+ New Preset" };
    juce::TextButton exportAllBtn { "Export User Presets" };
    std::unique_ptr<juce::FileChooser> fileChooser;

    static constexpr int ROW_H    = 19;
    static constexpr int HDR_H    = 22;
    static constexpr int COL_PAD  = 6;

    void buildPatches();
    int  rowAt(juce::Rectangle<int> col, juce::Point<int> p, int scroll) const;
    void drawColumn(juce::Graphics& g,
                    juce::Rectangle<int> bounds,
                    const char* title,
                    const juce::StringArray& items,
                    int selected, int hovered, int scroll,
                    juce::Colour accent) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowser)
};
