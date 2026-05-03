#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"

class WavetableDisplay : public juce::Component, private juce::Timer
{
public:
    explicit WavetableDisplay(VoidWaveAudioProcessor& p);
    ~WavetableDisplay() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    VoidWaveAudioProcessor& processor;

    // OSC mix controls
    juce::Slider    sOscMix;
    juce::ComboBox  cMixMode;
    juce::Label     lOsc1{"","OSC 1"}, lOsc2{"","OSC 2"}, lMix{"","MIX"};

    using SlAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CbAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<SlAtt> attMix;
    std::unique_ptr<CbAtt> attMode;

    // Animation
    float animPhase = 0.0f;

    // Param pointers (read in paint, set in constructor)
    std::atomic<float>* pOsc1Table    = nullptr;
    std::atomic<float>* pOsc1Pos     = nullptr;
    std::atomic<float>* pOsc2Table    = nullptr;
    std::atomic<float>* pOsc2Pos     = nullptr;
    std::atomic<float>* pOscMix      = nullptr;

    void timerCallback() override { animPhase += 0.015f; repaint(); }

    // Generate waveform samples for one cycle into a vector
    static std::vector<float> generateWave(int tableIndex, float positionBlend, int numSamples = 256);
    // Draw a waveform with multi-pass glow
    void drawGlowWave(juce::Graphics& g, const std::vector<float>& data,
                      juce::Colour col, juce::Rectangle<float> area,
                      float phaseOffset = 0.0f, float alpha = 1.0f) const;

    static juce::String tableName(int idx);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavetableDisplay)
};
