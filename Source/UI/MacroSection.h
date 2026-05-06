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

    // ── Sub oscillator + noise (top third) ────────────────────────────────
    juce::Slider  sSubLevel, sNoiseLevel, sNoiseColor;
    juce::Label   lSubLevel{"","SUB"}, lNoiseLevel{"","NOISE"}, lNoiseColor{"","COLOR"};
    juce::TextButton btnSubOct { "-1" };
    std::atomic<float>* pSubOctave = nullptr;

    // ── Character macros (bottom two-thirds) ──────────────────────────────
    static constexpr int N = 3;
    juce::Slider  sK[N];
    juce::Label   lK[N];

    using SlAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SlAtt> attSub, attNoiseLvl, attNoiseCol;
    std::unique_ptr<SlAtt> att[N];

    void updateOctBtn();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacroSection)
};
