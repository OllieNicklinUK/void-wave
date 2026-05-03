#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"

class ModMatrixSection : public juce::Component
{
public:
    explicit ModMatrixSection(VoidWaveAudioProcessor& p);
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    VoidWaveAudioProcessor& processor;
    static constexpr int SLOTS = 12;

    juce::ComboBox  cSrc[SLOTS], cDst[SLOTS];
    juce::Slider    sDepth[SLOTS];
    juce::ToggleButton tEnable[SLOTS];
    juce::Label     lNum[SLOTS];

    using SlAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CbAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BtAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SlAtt> attDepth[SLOTS];
    std::unique_ptr<CbAtt> attSrc[SLOTS], attDst[SLOTS];
    std::unique_ptr<BtAtt> attEnable[SLOTS];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModMatrixSection)
};
