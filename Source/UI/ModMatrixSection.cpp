#include "ModMatrixSection.h"
#include "LookAndFeel.h"

ModMatrixSection::ModMatrixSection(VoidWaveAudioProcessor& p) : processor(p)
{
    auto& t = p.apvts;
    auto styleC = [](juce::ComboBox& c)
    {
        c.setColour(juce::ComboBox::backgroundColourId, juce::Colour(VW::BG_RAISED));
        c.setColour(juce::ComboBox::textColourId,       juce::Colour(VW::TEXT_BRIGHT));
        c.setColour(juce::ComboBox::outlineColourId,    juce::Colour(VW::BORDER_VIS));
        c.setColour(juce::ComboBox::arrowColourId,      juce::Colour(0xff7a80a0));
    };

    // Source and dest are AudioParameterInt — populate manually
    static const juce::StringArray SRC_NAMES {
        "ENV 1","ENV 2","ENV 3","LFO 1","LFO 2",
        "VEL","NOTE","AFTERTOUCH","MOD WHEEL","BREATH",
        "MACRO 1","MACRO 2","MACRO 3","MACRO 4","MIDI CC"
    };
    static const juce::StringArray DST_NAMES {
        "OSC1 PITCH","OSC2 PITCH","OSC1 WT POS","OSC2 WT POS",
        "OSC1 LEVEL","OSC2 LEVEL","OSC1 PAN","OSC2 PAN",
        "OSC1 DETUNE","OSC2 DETUNE",
        "FILTER CUTOFF","FILTER RES","FILTER DRIVE",
        "LFO1 RATE","LFO2 RATE","LFO1 DEPTH","LFO2 DEPTH",
        "ENV1 DEPTH","AMP"
    };

    for (int i = 0; i < SLOTS; ++i)
    {
        styleC(cSrc[i]); addAndMakeVisible(cSrc[i]);
        styleC(cDst[i]); addAndMakeVisible(cDst[i]);
        cSrc[i].addItemList(SRC_NAMES, 1);
        cDst[i].addItemList(DST_NAMES, 1);

        sDepth[i].setSliderStyle(juce::Slider::LinearHorizontal);
        sDepth[i].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        sDepth[i].setColour(juce::Slider::thumbColourId,  juce::Colour(0xff7a80a0));
        sDepth[i].setColour(juce::Slider::trackColourId,  juce::Colour(VW::BORDER_VIS));
        addAndMakeVisible(sDepth[i]);

        tEnable[i].setColour(juce::ToggleButton::tickColourId,         juce::Colour(VW::NEON_CYAN));
        tEnable[i].setColour(juce::ToggleButton::tickDisabledColourId, juce::Colour(VW::BORDER_VIS));
        addAndMakeVisible(tEnable[i]);

        lNum[i].setText(juce::String::formatted("%02d", i + 1), juce::dontSendNotification);
        lNum[i].setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 9.0f, juce::Font::plain));
        lNum[i].setColour(juce::Label::textColourId, juce::Colour(0xff5a6080));
        lNum[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(lNum[i]);

        juce::String px = "mod" + juce::String(i + 1);
        attSrc   [i] = std::make_unique<CbAtt>(t, px + "_source",  cSrc   [i]);
        attDst   [i] = std::make_unique<CbAtt>(t, px + "_dest",    cDst   [i]);
        attDepth [i] = std::make_unique<SlAtt>(t, px + "_depth",   sDepth [i]);
        attEnable[i] = std::make_unique<BtAtt>(t, px + "_enabled", tEnable[i]);
    }
}

void ModMatrixSection::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(VW::BG_PANEL));
    g.setColour(juce::Colour(0xff4a5070));
    g.fillRect(0, 0, getWidth(), 14);
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xffffffff));
    g.drawText("MOD MATRIX", 0, 0, getWidth(), 14, juce::Justification::centred);
    g.setColour(juce::Colour(VW::BORDER_SUB));
    g.drawRect(getLocalBounds(), 1);
}

void ModMatrixSection::resized()
{
    const int W = getWidth(), H = getHeight();
    const int headerH = 18, rowH = (H - headerH) / SLOTS;
    const int numW = 20, enW = 16, depW = 50, pad = 4;
    int srcW = (W - numW - enW - depW - 5 * pad) / 2;
    int dstW = srcW;

    for (int i = 0; i < SLOTS; ++i)
    {
        int y = headerH + i * rowH + 1;
        int x = pad;
        lNum   [i].setBounds(x,               y, numW,  rowH - 2);  x += numW + pad;
        tEnable[i].setBounds(x,               y, enW,   rowH - 2);  x += enW  + pad;
        cSrc   [i].setBounds(x,               y, srcW,  rowH - 2);  x += srcW + pad;
        cDst   [i].setBounds(x,               y, dstW,  rowH - 2);  x += dstW + pad;
        sDepth [i].setBounds(x,               y, depW,  rowH - 2);
    }
}
