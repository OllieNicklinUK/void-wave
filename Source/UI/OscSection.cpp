#include "OscSection.h"
#include "LookAndFeel.h"

OscSection::OscSection(VoidWaveAudioProcessor& p,
                       const juce::String& pfx, juce::Colour acc)
    : processor(p), prefix(pfx), accent(acc)
{
    auto& t = p.apvts;
    auto iniK = [&](juce::Slider& s, juce::Label& l)
    {
        s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        s.setColour(juce::Slider::thumbColourId,               acc);
        s.setColour(juce::Slider::rotarySliderFillColourId,    acc);
        s.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(VW::BORDER_VIS));
        addAndMakeVisible(s);
        l.setJustificationType(juce::Justification::centred);
        l.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.0f, juce::Font::plain));
        l.setColour(juce::Label::textColourId, juce::Colour(VW::TEXT_MID));
        addAndMakeVisible(l);
    };
    iniK(sPos, lPos); iniK(sCoarse, lCoarse); iniK(sFine, lFine);
    iniK(sLevel, lLevel); iniK(sPan, lPan); iniK(sDetune, lDetune);
    iniK(sWidth, lWidth);
    if (pfx == "osc1") { iniK(sPitchAtk, lPitchAtk); iniK(sPitchAmt, lPitchAmt); }

    // TABLE: hidden slider, APVTS-bound. Value is now set from WavetableDisplay popup.
    sTable.setSliderStyle(juce::Slider::LinearHorizontal);
    sTable.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    sTable.setVisible(false);
    attTable = std::make_unique<SlAtt>(t, pfx + "_table", sTable);

    auto iniC = [&](juce::ComboBox& c, juce::Label& l)
    {
        c.setColour(juce::ComboBox::backgroundColourId, juce::Colour(VW::BG_RAISED));
        c.setColour(juce::ComboBox::textColourId,       juce::Colour(VW::TEXT_BRIGHT));
        c.setColour(juce::ComboBox::outlineColourId,    juce::Colour(VW::BORDER_VIS));
        c.setColour(juce::ComboBox::arrowColourId,      acc);
        addAndMakeVisible(c);
        l.setJustificationType(juce::Justification::centredLeft);
        l.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.0f, juce::Font::plain));
        l.setColour(juce::Label::textColourId, juce::Colour(VW::TEXT_MID));
        addAndMakeVisible(l);
    };
    iniC(cUnison, lUnison); iniC(cPhase, lPhase); iniC(cWTMode, lWTMode);

    // Populate combos from parameter choice strings
    auto pop = [&](juce::ComboBox& c, const juce::String& id) {
        if (auto* p = dynamic_cast<juce::AudioParameterChoice*>(t.getParameter(id)))
            c.addItemList(p->choices, 1);
    };
    pop(cUnison, pfx + "_unison");
    pop(cPhase,  pfx + "_phase_mode");
    pop(cWTMode, pfx + "_wt_mode");

    // attTable already created above in the table-nav block
    attPos    = std::make_unique<SlAtt>(t, pfx + "_position",   sPos);
    attCoarse = std::make_unique<SlAtt>(t, pfx + "_coarse",     sCoarse);
    attFine   = std::make_unique<SlAtt>(t, pfx + "_fine",       sFine);
    attLevel  = std::make_unique<SlAtt>(t, pfx + "_level",      sLevel);
    attPan    = std::make_unique<SlAtt>(t, pfx + "_pan",        sPan);
    attDetune = std::make_unique<SlAtt>(t, pfx + "_detune",     sDetune);
    attUnison = std::make_unique<CbAtt>(t, pfx + "_unison",     cUnison);
    attPhase  = std::make_unique<CbAtt>(t, pfx + "_phase_mode", cPhase);
    attWTMode = std::make_unique<CbAtt>(t, pfx + "_wt_mode",    cWTMode);

    attWidth = std::make_unique<SlAtt>(t, pfx + "_width", sWidth);
    sWidth.setComponentID(pfx + "_width");

    if (pfx == "osc1")
    {
        attPitchAtk = std::make_unique<SlAtt>(t, "pitchenv_attack", sPitchAtk);
        attPitchAmt = std::make_unique<SlAtt>(t, "pitchenv_amount", sPitchAmt);
        sPitchAtk.setComponentID("pitchenv_attack");
        sPitchAmt.setComponentID("pitchenv_amount");
    }

    // ComponentIDs — used by MIDI learn overlay
    sPos   .setComponentID(pfx + "_position"); sCoarse.setComponentID(pfx + "_coarse");
    sFine  .setComponentID(pfx + "_fine");     sLevel .setComponentID(pfx + "_level");
    sPan   .setComponentID(pfx + "_pan");      sDetune.setComponentID(pfx + "_detune");
}

void OscSection::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(VW::BG_PANEL));
    // Filled section header bar — GR-8 style
    g.setColour(accent);
    g.fillRect(0, 0, getWidth(), 14);
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xffffffff));
    g.drawText(prefix == "osc1" ? "OSC 1" : "OSC 2", 0, 0, getWidth(), 14, juce::Justification::centred);
    g.setColour(juce::Colour(VW::BORDER_SUB));
    g.drawRect(getLocalBounds(), 1);
}

void OscSection::resized()
{
    const int W = getWidth(), pad = 8;

    // Combos start immediately below header — no table nav row any more
    int cw = (W - 3 * pad) / 2;
    lUnison .setBounds(pad,          18, cw, 10); cUnison .setBounds(pad,          28, cw, 17);
    lPhase  .setBounds(pad+cw+pad,   18, cw, 10); cPhase  .setBounds(pad+cw+pad,   28, cw, 17);
    lWTMode .setBounds(pad,          48, cw*2+pad, 10); cWTMode.setBounds(pad,     58, cw*2+pad, 17);

    int kw = (W - 2 * pad) / 3;
    const int K = 38, Km = 28;
    auto plK = [&](juce::Slider& s, juce::Label& l, int col, int row, int sz)
    {
        int x = pad + col * kw + (kw - sz) / 2;
        int y = 96 + row * 56 + (K - sz) / 2;
        s.setBounds(x, y, sz, sz);
        l.setBounds(pad + col * kw, 96 + row * 56 + K + 2, kw, 10);
    };
    plK(sPos, lPos, 0, 0, K); plK(sCoarse, lCoarse, 1, 0, K); plK(sFine, lFine, 2, 0, K);
    plK(sLevel, lLevel, 0, 1, K); plK(sPan, lPan, 1, 1, K);   plK(sDetune, lDetune, 2, 1, Km);

    // Row 2: WIDTH (both), P.ATK + P.AMT (osc1 only)
    plK(sWidth, lWidth, 0, 2, Km);
    if (prefix == "osc1")
    {
        plK(sPitchAtk, lPitchAtk, 1, 2, Km);
        plK(sPitchAmt, lPitchAmt, 2, 2, Km);
    }
}
