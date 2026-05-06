#include "MacroSection.h"
#include "LookAndFeel.h"

static const juce::Colour AMBER { VW::NEON_AMBER };
static const juce::Colour COPPER { VW::SEC_LFO };

static const char* NAMES[]  = { "COLOUR", "SATURATION", "SPACE" };
static const char* MACROS[] = { "macro1", "macro2", "macro3" };

MacroSection::MacroSection(VoidWaveAudioProcessor& p) : processor(p)
{
    auto& t = p.apvts;

    // ── Sub oscillator knobs ──────────────────────────────────────────────
    auto iniK = [&](juce::Slider& s, juce::Label& l)
    {
        s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        s.setColour(juce::Slider::thumbColourId,               COPPER);
        s.setColour(juce::Slider::rotarySliderFillColourId,    COPPER);
        s.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(VW::BORDER_VIS));
        addAndMakeVisible(s);
        l.setJustificationType(juce::Justification::centred);
        l.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.0f, juce::Font::plain));
        l.setColour(juce::Label::textColourId, juce::Colour(VW::TEXT_MID));
        addAndMakeVisible(l);
    };
    iniK(sSubLevel,   lSubLevel);
    iniK(sNoiseLevel, lNoiseLevel);
    iniK(sNoiseColor, lNoiseColor);

    btnSubOct.setClickingTogglesState(false);
    btnSubOct.setColour(juce::TextButton::buttonColourId,  juce::Colour(VW::BG_CONTROL));
    btnSubOct.setColour(juce::TextButton::textColourOffId, COPPER.withAlpha(0.7f));
    btnSubOct.onClick = [this]
    {
        if (auto* param = dynamic_cast<juce::AudioParameterInt*>(
                processor.apvts.getParameter("sub_octave")))
        {
            int next = pSubOctave ? (static_cast<int>(pSubOctave->load()) == 0 ? 1 : 0) : 0;
            param->setValueNotifyingHost(param->getNormalisableRange().convertTo0to1(float(next)));
            updateOctBtn();
        }
    };
    addAndMakeVisible(btnSubOct);
    pSubOctave = t.getRawParameterValue("sub_octave");

    attSub      = std::make_unique<SlAtt>(t, "sub_level",   sSubLevel);
    attNoiseLvl = std::make_unique<SlAtt>(t, "noise_level", sNoiseLevel);
    attNoiseCol = std::make_unique<SlAtt>(t, "noise_color", sNoiseColor);
    updateOctBtn();

    // ── Character macro faders ────────────────────────────────────────────
    for (int i = 0; i < N; ++i)
    {
        sK[i].setSliderStyle(juce::Slider::LinearVertical);
        sK[i].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        sK[i].setColour(juce::Slider::thumbColourId,      AMBER);
        sK[i].setColour(juce::Slider::trackColourId,      AMBER.withAlpha(0.3f));
        sK[i].setColour(juce::Slider::backgroundColourId, juce::Colour(VW::BG_CONTROL));
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

void MacroSection::updateOctBtn()
{
    bool isTwo = pSubOctave && (static_cast<int>(pSubOctave->load()) == 1);
    btnSubOct.setButtonText(isTwo ? "-2" : "-1");
    btnSubOct.setColour(juce::TextButton::buttonColourId,
        isTwo ? COPPER.withAlpha(0.4f) : juce::Colour(VW::BG_CONTROL));
    btnSubOct.setColour(juce::TextButton::textColourOffId,
        isTwo ? COPPER : COPPER.withAlpha(0.6f));
    btnSubOct.repaint();
}

void MacroSection::paint(juce::Graphics& g)
{
    const int W = getWidth(), pad = 8;

    // Sub Osc section title
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.0f, juce::Font::bold));
    g.setColour(COPPER.withAlpha(0.75f));
    g.drawText("SUB OSC", pad, 1, W - 2*pad, 11, juce::Justification::left);

    // Divider
    const int divY = 108;
    g.setColour(COPPER.withAlpha(0.2f));
    g.drawHorizontalLine(divY, static_cast<float>(pad), static_cast<float>(W - pad));

    // Character title
    g.setColour(AMBER.withAlpha(0.75f));
    g.drawText("CHARACTER", pad, divY + 4, W - 2*pad, 11, juce::Justification::left);
}

void MacroSection::resized()
{
    const int W = getWidth(), H = getHeight(), pad = 8;

    // ── Sub oscillator area (top ~105px) ─────────────────────────────────
    const int Ks = 46;  // match OSC section knob size
    const int numSubSlots = 4;
    const int subKw = (W - 2*pad) / numSubSlots;

    // Slot 0: Sub Level knob
    {
        int x = pad + 0*subKw + (subKw-Ks)/2;
        sSubLevel.setBounds(x, 18, Ks, Ks);
        lSubLevel.setBounds(pad + 0*subKw, 18+Ks+2, subKw, 10);
    }
    // Slot 1: Octave button (centred vertically in knob area)
    {
        int x = pad + 1*subKw + (subKw-28)/2;
        btnSubOct.setBounds(x, 18+(Ks-16)/2, 28, 16);
    }
    // Slot 2: Noise Level knob
    {
        int x = pad + 2*subKw + (subKw-Ks)/2;
        sNoiseLevel.setBounds(x, 18, Ks, Ks);
        lNoiseLevel.setBounds(pad + 2*subKw, 18+Ks+2, subKw, 10);
    }
    // Slot 3: Noise Color knob
    {
        int x = pad + 3*subKw + (subKw-Ks)/2;
        sNoiseColor.setBounds(x, 18, Ks, Ks);
        lNoiseColor.setBounds(pad + 3*subKw, 18+Ks+2, subKw, 10);
    }

    // ── Character faders (below divider) ─────────────────────────────────
    const int faderTop = 124;
    const int labelH = 13;
    const int fw = (W - 2 * pad) / N;
    const int faderW = juce::jmax(8, fw - 8);

    for (int i = 0; i < N; ++i)
    {
        int x = pad + i * fw + (fw - faderW) / 2;
        sK[i].setBounds(x, faderTop, faderW, H - faderTop - labelH - pad);
        lK[i].setBounds(pad + i * fw, H - labelH - 2, fw, labelH);
    }
}
