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

    // TABLE: hidden slider + 9 visible wave-shape selector buttons
    sTable.setSliderStyle(juce::Slider::LinearHorizontal);
    sTable.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    sTable.setVisible(false);
    attTable = std::make_unique<SlAtt>(t, pfx + "_table", sTable);
    pTable  = t.getRawParameterValue(pfx + "_table");
    pWTMode = t.getRawParameterValue(pfx + "_wt_mode");

    static const char* WAVE_LABELS[] = {
        "SIN","SAW","SQR","SPC","F.A","F.E","BS1"
    };
    for (int i = 0; i < N_WAVE; ++i)
    {
        waveBtn[i].setButtonText(WAVE_LABELS[i]);
        waveBtn[i].setClickingTogglesState(false);
        waveBtn[i].setColour(juce::TextButton::buttonColourId,   juce::Colour(VW::BG_RAISED));
        waveBtn[i].setColour(juce::TextButton::buttonOnColourId, acc.withAlpha(0.22f));
        waveBtn[i].setColour(juce::TextButton::textColourOffId,  juce::Colour(VW::TEXT_MID));
        waveBtn[i].setColour(juce::TextButton::textColourOnId,   acc);
        int idx = i;
        waveBtn[i].onClick = [this, idx]()
        {
            if (auto* param = processor.apvts.getParameter(prefix + "_table"))
                param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(idx)));
        };
        addAndMakeVisible(waveBtn[i]);
    }

    // SCAN toggle button
    scanBtn.setClickingTogglesState(false);
    scanBtn.setColour(juce::TextButton::buttonColourId,   juce::Colour(VW::BG_RAISED));
    scanBtn.setColour(juce::TextButton::buttonOnColourId, acc.withAlpha(0.35f));
    scanBtn.setColour(juce::TextButton::textColourOffId,  juce::Colour(VW::TEXT_MID));
    scanBtn.setColour(juce::TextButton::textColourOnId,   acc);
    scanBtn.onClick = [this]()
    {
        if (auto* param = processor.apvts.getParameter(prefix + "_wt_mode"))
        {
            int cur = static_cast<int>(std::round(param->convertFrom0to1(param->getValue())));
            int next = (cur == 2) ? 0 : 2;   // toggle SCAN (2) ↔ SINGLE (0)
            param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(next)));
        }
    };
    addAndMakeVisible(scanBtn);

    startTimerHz(30);

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

void OscSection::paint(juce::Graphics&) {}

void OscSection::timerCallback()
{
    if (pTable)
    {
        int cur = static_cast<int>(std::round(pTable->load()));
        for (int i = 0; i < N_WAVE; ++i)
        {
            bool active = (i == cur);
            waveBtn[i].setColour(juce::TextButton::buttonColourId,
                                 active ? accent.withAlpha(0.22f)
                                        : juce::Colour(VW::BG_RAISED));
            waveBtn[i].setColour(juce::TextButton::textColourOffId,
                                 active ? accent : juce::Colour(VW::TEXT_MID));
        }
    }

    if (pWTMode)
    {
        bool isScan = (static_cast<int>(std::round(pWTMode->load())) == 2);
        scanBtn.setColour(juce::TextButton::buttonColourId,
                          isScan ? accent.withAlpha(0.35f)
                                 : juce::Colour(VW::BG_RAISED));
        scanBtn.setColour(juce::TextButton::textColourOffId,
                          isScan ? accent : juce::Colour(VW::TEXT_MID));
    }
}

void OscSection::resized()
{
    const int W = getWidth(), pad = 8;

    // Combos start immediately below header
    int cw = (W - 3 * pad) / 2;
    lUnison .setBounds(pad,          18, cw, 10); cUnison .setBounds(pad,          28, cw, 17);
    lPhase  .setBounds(pad+cw+pad,   18, cw, 10); cPhase  .setBounds(pad+cw+pad,   28, cw, 17);
    lWTMode .setBounds(pad,          48, cw*2+pad, 10); cWTMode.setBounds(pad,     58, cw*2+pad, 17);

    // Wave shape row: 9 equal slots total.
    // Slots 0–6 = wave buttons (SIN…BS1), slots 7–8 = SCAN (double-wide).
    {
        const int totalSlots = 9;
        const int slotW = (W - 2 * pad) / totalSlots;
        for (int i = 0; i < N_WAVE; ++i)
            waveBtn[i].setBounds(pad + i * slotW, 78, slotW - 1, 15);
        // SCAN spans the last 2 slots — exact same x and width as the removed buttons
        int scanX = pad + N_WAVE * slotW;
        int scanW = slotW * 2 - 1;
        scanBtn.setBounds(scanX, 78, scanW, 15);
    }

    int kw = (W - 2 * pad) / 3;
    const int K = 46, Km = 34, SPACING = 66, KNOB_TOP = 106;  // +10px top margin
    auto plK = [&](juce::Slider& s, juce::Label& l, int col, int row, int sz)
    {
        int x = pad + col * kw + (kw - sz) / 2;
        int y = KNOB_TOP + row * SPACING + (K - sz) / 2;
        s.setBounds(x, y, sz, sz);
        l.setBounds(pad + col * kw, KNOB_TOP + row * SPACING + K + 2, kw, 10);
    };
    plK(sPos, lPos, 0, 0, K); plK(sCoarse, lCoarse, 1, 0, K); plK(sFine, lFine, 2, 0, K);
    plK(sLevel, lLevel, 0, 1, K); plK(sPan, lPan, 1, 1, K);   plK(sDetune, lDetune, 2, 1, K);

    // Row 2: same 3-column grid as rows 0 and 1 — aligns with knobs above
    plK(sWidth, lWidth, 0, 2, K);
    if (prefix == "osc1")
    {
        plK(sPitchAtk, lPitchAtk, 1, 2, K);
        plK(sPitchAmt, lPitchAmt, 2, 2, K);
    }

}
