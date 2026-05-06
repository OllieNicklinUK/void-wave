#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"

class FilterSection : public juce::Component, private juce::Timer
{
public:
    explicit FilterSection(VoidWaveAudioProcessor& p);
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    VoidWaveAudioProcessor& processor;

    juce::Slider    sCutoff, sRes, sDrive, sKeyTrack, sVelTrack, sEnvDepth;
    juce::ComboBox  cType;
    juce::Label     lCutoff{"","CUTOFF"}, lRes{"","RES"}, lDrive{"","DRIVE"},
                    lKey{"","KEY TRK"}, lVel{"","VEL TRK"}, lType{"","TYPE"},
                    lEnvDepth{"","ENV DEP"};

    // OSC filter route buttons: 0=Both(1+2), 1=OSC1, 2=OSC2
    juce::TextButton btnRoute[3];
    std::atomic<float>* pFilterRoute = nullptr;
    void updateRouteButtons();
    void setRoute(int route);

    juce::Label sectionTitle;

    // ENV1 param pointers — for live ADSR visualisation
    std::atomic<float>* pEnv1Atk  = nullptr;
    std::atomic<float>* pEnv1Dcy  = nullptr;
    std::atomic<float>* pEnv1Sus  = nullptr;
    std::atomic<float>* pEnv1Hld  = nullptr;
    std::atomic<float>* pEnv1Rel  = nullptr;
    std::atomic<float>* pEnv1Dep  = nullptr;

    void timerCallback() override { updateRouteButtons(); repaint(); }

    // Draw ADSR shape (shared static helper used by Filter and Envelope)
    static void drawADSR(juce::Graphics& g, juce::Rectangle<float> area,
                         float atk, float hld, float dcy, float sus, float rel,
                         juce::Colour col);

    using SlAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CbAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<SlAtt> attCutoff, attRes, attDrive, attKey, attVel, attEnvDepth;
    std::unique_ptr<CbAtt> attType;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterSection)
};
