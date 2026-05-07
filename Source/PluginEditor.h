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
#include "UI/PresetBrowser.h"

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

    // Preset bar — prev/next + name display button + import
    juce::TextButton prevBtn { "<" }, nextBtn { ">" };
    juce::TextButton presetNameBtn;          // shows current preset, click to open browser
    juce::TextButton importBtn { "IMPORT" };
    juce::TextButton modMatrixBtn { "MATRIX" };
    std::unique_ptr<juce::FileChooser> fileChooser;

    // Three-column browser (shown as overlay)
    PresetBrowser presetBrowser;

    // MIDI learn
    juce::TextButton midiMapBtn { "MIDI MAP" };
    MidiLearnOverlay midiLearnOverlay;

    // Save preset
    juce::TextButton saveBtn { "SAVE" };

    // Auto-play (standalone only)
    juce::TextButton autoPlayBtn { "AUTO" };
    juce::Label      noteLabel;

    // Master volume knob (top bar)
    juce::Slider sMaster;
    juce::Label  lMaster { "", "MASTER" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attMaster;

    void timerCallback() override;
    void updatePresetDisplay();
    void showBrowser();
    void hideBrowser();
    void onSavePreset();
    static juce::String midiNoteToName(int note);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoidWaveAudioProcessorEditor)
};
