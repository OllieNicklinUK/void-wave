#include "MacroSection.h"
#include "LookAndFeel.h"
static const juce::Colour AMBER { VW::NEON_AMBER };
static const char* MACRO_NAMES[] = { "DRIVE", "CROSS" };

MacroSection::MacroSection(VoidWaveAudioProcessor& p) : processor(p)
{
    auto& t = p.apvts;
    for (int i = 0; i < 2; ++i)
    {
        // Vertical fader matching the reference UI
        sM[i].setSliderStyle(juce::Slider::LinearVertical);
        sM[i].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        sM[i].setColour(juce::Slider::thumbColourId,        AMBER);
        sM[i].setColour(juce::Slider::trackColourId,        AMBER.withAlpha(0.25f));
        sM[i].setColour(juce::Slider::backgroundColourId,   juce::Colour(VW::BG_CONTROL));
        addAndMakeVisible(sM[i]);
        lM[i].setText(MACRO_NAMES[i], juce::dontSendNotification);
        lM[i].setJustificationType(juce::Justification::centred);
        lM[i].setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.0f, juce::Font::plain));
        lM[i].setColour(juce::Label::textColourId, juce::Colour(VW::TEXT_MID));
        addAndMakeVisible(lM[i]);
        att[i] = std::make_unique<SlAtt>(t, "macro" + juce::String(i + 1), sM[i]);
        sM[i].setComponentID("macro" + juce::String(i + 1));
    }
}

void MacroSection::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(VW::BG_PANEL));
    g.setColour(AMBER);
    g.fillRect(0, 0, getWidth(), 14);
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xffffffff));
    g.drawText("MACRO", 0, 0, getWidth(), 14, juce::Justification::centred);
    g.setColour(juce::Colour(VW::BORDER_SUB));
    g.drawRect(getLocalBounds(), 1);
}

void MacroSection::resized()
{
    const int W = getWidth(), H = getHeight(), pad = 16;
    const int faderW = 28, labelH = 14;
    int totalW = 2 * faderW + pad;
    int startX = (W - totalW) / 2;
    int faderH = H - 18 - labelH - pad;

    for (int i = 0; i < 2; ++i)
    {
        int x = startX + i * (faderW + pad);
        sM[i].setBounds(x, 18, faderW, faderH);
        lM[i].setBounds(x - 4, H - labelH - 2, faderW + 8, labelH);
    }
}
