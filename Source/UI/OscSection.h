#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"

class OscSection : public juce::Component
{
public:
    OscSection(VoidWaveAudioProcessor& p, const juce::String& paramPrefix, juce::Colour accent);
    ~OscSection() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    VoidWaveAudioProcessor& processor;
    juce::String prefix;
    juce::Colour accent;

    juce::Slider  sPos, sCoarse, sFine, sLevel, sPan, sDetune;
    juce::Slider  sWidth, sPitchAtk, sPitchAmt;
    juce::Slider  sTable;   // hidden, APVTS-bound; value driven by arrows
    juce::ComboBox cUnison, cPhase, cWTMode;

    // Wavetable type cycle UI
    juce::TextButton tablePrev{"<"}, tableNext{">"};
    juce::Label      tableNameLbl;

    juce::Label lPos{"","POS"}, lCoarse{"","COARSE"}, lFine{"","FINE"},
                lLevel{"","LEVEL"}, lPan{"","PAN"}, lDetune{"","DETUNE"},
                lUnison{"","UNISON"}, lPhase{"","PHASE"}, lWTMode{"","MODE"},
                lWidth{"","WIDTH"}, lPitchAtk{"","P.ATK"}, lPitchAmt{"","P.AMT"};

    using SlAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CbAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<SlAtt> attPos, attCoarse, attFine, attLevel, attPan, attDetune, attTable;
    std::unique_ptr<SlAtt> attWidth, attPitchAtk, attPitchAmt;
    std::unique_ptr<CbAtt> attUnison, attPhase, attWTMode;

    void styleKnob(juce::Slider& s, juce::Label& l, int px);
    void styleCombo(juce::ComboBox& c, juce::Label& l);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscSection)
};
