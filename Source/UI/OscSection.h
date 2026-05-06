#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"

class OscSection : public juce::Component,
                   private juce::Timer
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

    // Wave shape selector buttons (SINE SAW SQR SPEC F.A F.E BS1 BS2 NSE)
    static constexpr int N_WAVE = 7;  // SIN SAW SQR SPC F.A F.E BS1  (BS2+NSE replaced by SCAN)
    juce::TextButton waveBtn[N_WAVE];
    std::atomic<float>* pTable  = nullptr;

    // SCAN toggle button
    juce::TextButton scanBtn { "SCAN" };
    std::atomic<float>* pWTMode = nullptr;

    // WAV import
    juce::TextButton loadWavBtn { "LOAD WAV" };
    std::shared_ptr<juce::FileChooser> fileChooser;
    void triggerWavLoad();

    // Section title
    juce::Label titleLabel;

    juce::Slider  sPos, sCoarse, sFine, sLevel, sPan, sDetune;
    juce::Slider  sWidth, sPitchAtk, sPitchAmt;
    juce::Slider  sTable;   // hidden, APVTS-bound
    juce::ComboBox cUnison, cPhase, cWTMode;

    juce::Label lPos{"","POS"}, lCoarse{"","COARSE"}, lFine{"","FINE"},
                lLevel{"","LEVEL"}, lPan{"","PAN"}, lDetune{"","DETUNE"},
                lUnison{"","UNISON"}, lPhase{"","PHASE"}, lWTMode{"","MODE"},
                lWidth{"","WIDTH"}, lPitchAtk{"","P.ATK"}, lPitchAmt{"","P.AMT"};

    using SlAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CbAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<SlAtt> attPos, attCoarse, attFine, attLevel, attPan, attDetune, attTable;
    std::unique_ptr<SlAtt> attWidth, attPitchAtk, attPitchAmt;
    std::unique_ptr<CbAtt> attUnison, attPhase, attWTMode;

    // Timer: sync button highlight to current APVTS table value
    void timerCallback() override;

    void styleKnob(juce::Slider& s, juce::Label& l, int px);
    void styleCombo(juce::ComboBox& c, juce::Label& l);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscSection)
};
