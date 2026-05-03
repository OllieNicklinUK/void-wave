#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"

class EnvelopeSection : public juce::Component, private juce::Timer
{
public:
    explicit EnvelopeSection(VoidWaveAudioProcessor& p);
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    VoidWaveAudioProcessor& processor;
    int currentEnv = 0;

    juce::TextButton tabBtns[3];

    // ENV 1 (with depth)
    juce::Slider sAtk1,sDcy1,sSus1,sHld1,sRel1,sDep1;
    juce::Label  lAtk1{"","ATK"},lDcy1{"","DCY"},lSus1{"","SUS"},
                 lHld1{"","HLD"},lRel1{"","REL"},lDep1{"","DEPTH"};
    juce::ComboBox cCrv1; juce::Label lCrv1{"","CURVE"};
    // ENV 2
    juce::Slider sAtk2,sDcy2,sSus2,sHld2,sRel2;
    juce::Label  lAtk2{"","ATK"},lDcy2{"","DCY"},lSus2{"","SUS"},
                 lHld2{"","HLD"},lRel2{"","REL"};
    juce::ComboBox cCrv2; juce::Label lCrv2{"","CURVE"};
    // ENV 3
    juce::Slider sAtk3,sDcy3,sSus3,sHld3,sRel3;
    juce::Label  lAtk3{"","ATK"},lDcy3{"","DCY"},lSus3{"","SUS"},
                 lHld3{"","HLD"},lRel3{"","REL"};
    juce::ComboBox cCrv3; juce::Label lCrv3{"","CURVE"};

    using SlAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CbAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<SlAtt> aAtk1,aDcy1,aSus1,aHld1,aRel1,aDep1;
    std::unique_ptr<SlAtt> aAtk2,aDcy2,aSus2,aHld2,aRel2;
    std::unique_ptr<SlAtt> aAtk3,aDcy3,aSus3,aHld3,aRel3;
    std::unique_ptr<CbAtt> aCrv1,aCrv2,aCrv3;

    void showEnv(int idx);
    void iniKnob(juce::Slider& s, juce::Label& l);
    void iniCombo(juce::ComboBox& c);
    void timerCallback() override { repaint(); }
    void layoutEnv(int envIdx, juce::Rectangle<int> area);

    // Raw param pointers for live ADSR visualisation
    std::atomic<float>* envPtrs[3][6] = {};  // [envIdx][atk,dcy,sus,hld,rel,dep]

    static void drawADSR(juce::Graphics& g, juce::Rectangle<float> area,
                         float atk, float hld, float dcy, float sus, float rel,
                         juce::Colour col);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeSection)
};
