#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"

class FXSection : public juce::Component
{
public:
    explicit FXSection(VoidWaveAudioProcessor& p);
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    VoidWaveAudioProcessor& processor;

    // FX 1 — Distortion
    juce::ComboBox cFX1Type;
    juce::Slider   sFX1Drive, sFX1Tone, sFX1Mix;
    juce::Label    lFX1{"","DISTORTION"}, lFX1Drive{"","DRIVE"}, lFX1Tone{"","TONE"}, lFX1Mix{"","MIX"};

    // FX 2 — Modulation
    juce::ComboBox cFX2Type;
    juce::Slider   sFX2Rate, sFX2Depth, sFX2Mix;
    juce::Label    lFX2{"","MODULATION"}, lFX2Rate{"","RATE"}, lFX2Depth{"","DEPTH"}, lFX2Mix{"","MIX"};

    // FX 3 — Delay
    juce::ComboBox  cFX3Type;
    juce::Slider    sFX3Time, sFX3Fbk, sFX3Mix;
    juce::TextButton tFX3Sync { "SYNC" };
    juce::Label     lFX3{"","DELAY"}, lFX3Time{"","TIME"}, lFX3Fbk{"","FEEDBK"}, lFX3Mix{"","MIX"};

    // FX 4 — Reverb (type not in APVTS — sets knob values on select)
    juce::ComboBox cFX4Type;
    juce::Slider   sFX4Size, sFX4Damp, sFX4Pre, sFX4Mix;
    juce::Label    lFX4{"","REVERB"}, lFX4Type{"","TYPE"},
                   lFX4Size{"","SIZE"}, lFX4Damp{"","DAMP"}, lFX4Pre{"","PRE"}, lFX4Mix{"","MIX"};

    using SlAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CbAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BtAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SlAtt> aFX1Drv, aFX1Ton, aFX1Mix;
    std::unique_ptr<SlAtt> aFX2Rat, aFX2Dep, aFX2Mix;
    std::unique_ptr<SlAtt> aFX3Tim, aFX3Fbk, aFX3Mix;
    std::unique_ptr<SlAtt> aFX4Siz, aFX4Dam, aFX4Pre, aFX4Mix;
    std::unique_ptr<CbAtt> aFX1Typ, aFX2Typ, aFX3Typ;
    std::unique_ptr<BtAtt> aFX3Sync;

    void applyReverbPreset(int idx);   // 0=Room 1=Hall 2=Plate

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FXSection)
};
