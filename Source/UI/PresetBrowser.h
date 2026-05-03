#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"

class PresetBrowser : public juce::Component
{
public:
    explicit PresetBrowser(VoidWaveAudioProcessor& p);
    ~PresetBrowser() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    VoidWaveAudioProcessor& processor;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowser)
};
