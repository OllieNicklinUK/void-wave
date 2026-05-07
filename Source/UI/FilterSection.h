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

    std::atomic<float>* pFilterType = nullptr;
    std::atomic<float>* pFilterCutoff = nullptr;
    std::atomic<float>* pFilterRes = nullptr;

    void drawFrequencyResponse(juce::Graphics& g, juce::Rectangle<float> area);

    void timerCallback() override { updateRouteButtons(); repaint(); }

    using SlAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CbAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<SlAtt> attCutoff, attRes, attDrive, attKey, attVel, attEnvDepth;
    std::unique_ptr<CbAtt> attType;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterSection)
};
