#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"

// Character controls: COLOUR / SATURATION / SPACE
// Each maps to macro1/2/3 in the APVTS and is applied as a broad musical gesture.
class MacroSection : public juce::Component
{
public:
    explicit MacroSection(VoidWaveAudioProcessor& p);
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    VoidWaveAudioProcessor& processor;

    static constexpr int N = 3;
    juce::Slider  sK[N];
    juce::Label   lK[N];

    using SlAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SlAtt> att[N];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacroSection)
};
