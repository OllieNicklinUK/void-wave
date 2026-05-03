#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"

// Transparent overlay that:
//  - draws highlights around mapped / selected controls
//  - uses a MouseListener to capture clicks and set midiLearnTarget
class MidiLearnOverlay : public juce::Component
{
public:
    explicit MidiLearnOverlay(VoidWaveAudioProcessor& p, juce::Component& root);
    ~MidiLearnOverlay() override;

    void paint(juce::Graphics&) override;

    // Called when learn mode toggles on/off
    void setActive(bool active);

private:
    VoidWaveAudioProcessor& processor;
    juce::Component&        rootComp;

    void mouseDown(const juce::MouseEvent& e) override;

    // Recursive helpers
    void drawHighlights(juce::Graphics& g, juce::Component* node) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiLearnOverlay)
};
