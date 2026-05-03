#include "PresetBrowser.h"

PresetBrowser::PresetBrowser(VoidWaveAudioProcessor& p) : processor(p) {}

void PresetBrowser::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(0xff13151a));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);
    g.setColour(juce::Colour(0xff2a2d35));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 4.0f, 1.0f);
}

void PresetBrowser::resized() {}
