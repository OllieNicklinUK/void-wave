#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"

class LFOSection : public juce::Component, private juce::Timer
{
public:
    explicit LFOSection(VoidWaveAudioProcessor& p);
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    VoidWaveAudioProcessor& processor;
    int currentLFO = 0;
    juce::TextButton tabBtns[2];

    juce::Slider  sRate1,sDepth1,sPhase1,sFade1,sSyncDiv1;
    juce::Slider  sRate2,sDepth2,sPhase2,sFade2,sSyncDiv2;
    juce::Label   lRate1{"","RATE"},lDepth1{"","DEPTH"},lPhase1{"","PHASE"},
                  lFade1{"","FADE"},lSyncDiv1{"","DIV"};
    juce::Label   lRate2{"","RATE"},lDepth2{"","DEPTH"},lPhase2{"","PHASE"},
                  lFade2{"","FADE"},lSyncDiv2{"","DIV"};
    juce::ComboBox   cShape1,cTrig1,cShape2,cTrig2;
    juce::TextButton tSync1{"SYNC"}, tSync2{"SYNC"};
    juce::Label   lShape1{"","SHAPE"},lTrig1{"","TRIG"},
                  lShape2{"","SHAPE"},lTrig2{"","TRIG"};

    using SlAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CbAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BtAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SlAtt> aRate1,aDepth1,aPhase1,aFade1,aDiv1;
    std::unique_ptr<SlAtt> aRate2,aDepth2,aPhase2,aFade2,aDiv2;
    std::unique_ptr<CbAtt> aShape1,aTrig1,aShape2,aTrig2;
    std::unique_ptr<BtAtt> aSync1, aSync2;

    // LFO preview animation
    float lfoAnimPhase = 0.0f;
    std::atomic<float>* pShape1 = nullptr;
    std::atomic<float>* pShape2 = nullptr;
    std::atomic<float>* pRate1  = nullptr;
    std::atomic<float>* pRate2  = nullptr;

    void timerCallback() override { lfoAnimPhase += 0.04f; repaint(); }
    static float lfoSample(int shape, float phase);  // 0-1 phase → -1..1
    void drawLFOPreview(juce::Graphics& g, juce::Rectangle<int> area) const;

    void showLFO(int idx);
    void iniKnob(juce::Slider& s, juce::Label& l);
    void iniCombo(juce::ComboBox& c);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LFOSection)
};
