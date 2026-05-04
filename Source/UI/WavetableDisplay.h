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
    void mouseUp(const juce::MouseEvent&) override;

private:
    VoidWaveAudioProcessor& processor;

    // OSC mix controls
    juce::Slider   sOscMix;
    juce::ComboBox cMixMode;
    juce::Label    lMix{"","MIX"};

    using SlAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CbAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<SlAtt> attMix;
    std::unique_ptr<CbAtt> attMode;

    // Param pointers
    std::atomic<float>* pOsc1Table  = nullptr;
    std::atomic<float>* pOsc1Pos   = nullptr;
    std::atomic<float>* pOsc1Mode  = nullptr;   // wt_mode (SCAN = 2)
    std::atomic<float>* pOsc2Table  = nullptr;
    std::atomic<float>* pOsc2Pos   = nullptr;
    std::atomic<float>* pOsc2Mode  = nullptr;
    std::atomic<float>* pOscMix    = nullptr;

    // 3D waterfall: cached frames per oscillator
    static constexpr int N_FRAMES     = 12;
    static constexpr int WAVE_SAMPLES = 64;
    std::vector<std::vector<float>> frames1, frames2;
    int cachedTable1 = -1, cachedTable2 = -1;
    void rebuildCache(int tableIdx, std::vector<std::vector<float>>& cache);

    // Animated scan display position (0–1, advances in SCAN mode)
    float scanPos1 = 0.0f, scanPos2 = 0.0f;
    float animTick = 0.0f;

    // Click zones (set in paint, used in mouseUp)
    juce::Rectangle<int> osc1NameZone, osc2NameZone;

    void timerCallback() override;
    void showTableMenu(bool isOsc1);

    static std::vector<float> generateWave(int tableIndex, float pos, int n = WAVE_SAMPLES);
    static juce::String tableName(int idx);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavetableDisplay)
};
