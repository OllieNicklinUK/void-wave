#include "MacroSection.h"
#include "LookAndFeel.h"

static const juce::Colour AMBER { VW::NEON_AMBER };

static const char* NAMES[]  = { "COLOUR", "SATURATION", "SPACE" };
static const char* MACROS[] = { "macro1", "macro2",     "macro3" };

MacroSection::MacroSection(VoidWaveAudioProcessor& p) : processor(p)
{
    auto& t = p.apvts;
    for (int i = 0; i < N; ++i)
    {
        sK[i].setSliderStyle(juce::Slider::RotaryVerticalDrag);
        sK[i].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        sK[i].setColour(juce::Slider::thumbColourId,               AMBER);
        sK[i].setColour(juce::Slider::rotarySliderFillColourId,    AMBER);
        sK[i].setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(VW::BORDER_VIS));
        addAndMakeVisible(sK[i]);
        sK[i].setComponentID(MACROS[i]);

        lK[i].setText(NAMES[i], juce::dontSendNotification);
        lK[i].setJustificationType(juce::Justification::centred);
        lK[i].setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 7.5f, juce::Font::plain));
        lK[i].setColour(juce::Label::textColourId, juce::Colour(VW::TEXT_MID));
        addAndMakeVisible(lK[i]);

        att[i] = std::make_unique<SlAtt>(t, MACROS[i], sK[i]);
    }
}

void MacroSection::paint(juce::Graphics&) {}

void MacroSection::resized()
{
    const int W = getWidth(), H = getHeight(), pad = 6;
    const int K = juce::jmin(44, (W - 2 * pad) / N - 4);
    int kw = (W - 2 * pad) / N;

    for (int i = 0; i < N; ++i)
    {
        int cx = pad + i * kw + (kw - K) / 2;
        int cy = 18 + (H - 18 - K - 16) / 2;
        sK[i].setBounds(cx, cy, K, K);
        lK[i].setBounds(pad + i * kw, cy + K + 2, kw, 12);
    }
}
