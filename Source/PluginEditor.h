#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/LookAndFeel.h"
#include "UI/OscSection.h"
#include "UI/WavetableDisplay.h"
#include "UI/FilterSection.h"
#include "UI/EnvelopeSection.h"
#include "UI/ModMatrixSection.h"
#include "UI/LFOSection.h"
#include "UI/MacroSection.h"
#include "UI/FXSection.h"
#include "UI/MidiLearnOverlay.h"

class VoidWaveAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer
{
public:
    explicit VoidWaveAudioProcessorEditor(VoidWaveAudioProcessor&);
    ~VoidWaveAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    VoidWaveAudioProcessor& audioProcessor;
    VoidWaveLookAndFeel     lookAndFeel;

    // Sections
    OscSection       osc1Section;
    OscSection       osc2Section;
    WavetableDisplay wtDisplay;
    FilterSection    filterSection;
    EnvelopeSection  envSection;
    ModMatrixSection modMatrix;
    LFOSection       lfoSection;
    MacroSection     macroSection;
    FXSection        fxSection;

    // Preset bar — category filter + preset selector
    juce::TextButton prevBtn { "<" }, nextBtn { ">" };
    juce::ComboBox   catCombo;      // category filter
    juce::ComboBox   presetCombo;   // presets in selected category
    juce::Label      statusLabel;

    // Maps presetCombo item IDs to PresetManager indices
    juce::Array<int> presetComboMap;

    // Preset import
    juce::TextButton importBtn { "IMPORT" };
    std::unique_ptr<juce::FileChooser> fileChooser;

    // MIDI learn
    juce::TextButton     midiMapBtn { "MIDI MAP" };
    MidiLearnOverlay     midiLearnOverlay;

    // Save preset
    juce::TextButton saveBtn { "SAVE" };

    // Auto-play (standalone only)
    juce::TextButton autoPlayBtn { "AUTO" };
    juce::Label      noteLabel;

    void timerCallback() override;
    void populatePresetCombos();           // rebuild from PresetManager
    void onCategoryChanged();              // catCombo changed
    void onPresetSelected();               // presetCombo changed
    void updatePresetDisplay();            // sync combos to currentIndex
    void onSavePreset();                   // save current params as named preset
    static juce::String midiNoteToName(int note);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoidWaveAudioProcessorEditor)
};
