#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"

class MacroSection : public juce::Component
{
public:
    explicit MacroSection(VoidWaveAudioProcessor& p);
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    VoidWaveAudioProcessor& processor;
    juce::Slider  sM[2];
    juce::Label   lM[2];
    using SlAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SlAtt> att[2];
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacroSection)
};
