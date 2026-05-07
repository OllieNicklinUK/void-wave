#include "LFOSection.h"
#include "LookAndFeel.h"
static const juce::Colour PURPLE { VW::SEC_LFO };

void LFOSection::iniKnob(juce::Slider& s, juce::Label& l)
{
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    s.setColour(juce::Slider::thumbColourId,               PURPLE);
    s.setColour(juce::Slider::rotarySliderFillColourId,    PURPLE);
    s.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(VW::BORDER_VIS));
    addAndMakeVisible(s);
    l.setJustificationType(juce::Justification::centred);
    l.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.0f, juce::Font::plain));
    l.setColour(juce::Label::textColourId, juce::Colour(VW::TEXT_MID));
    addAndMakeVisible(l);
}
void LFOSection::iniCombo(juce::ComboBox& c)
{
    c.setColour(juce::ComboBox::backgroundColourId, juce::Colour(VW::BG_RAISED));
    c.setColour(juce::ComboBox::textColourId,       juce::Colour(VW::TEXT_BRIGHT));
    c.setColour(juce::ComboBox::outlineColourId,    juce::Colour(VW::BORDER_VIS));
    c.setColour(juce::ComboBox::arrowColourId,      PURPLE);
    addAndMakeVisible(c);
}

LFOSection::LFOSection(VoidWaveAudioProcessor& p) : processor(p)
{
    auto& t = p.apvts;
    const char* tabs[] = { "LFO 1", "LFO 2" };
    for (int i = 0; i < 2; ++i)
    {
        tabBtns[i].setButtonText(tabs[i]);
        tabBtns[i].setColour(juce::TextButton::buttonColourId,   juce::Colour(VW::BG_RAISED));
        tabBtns[i].setColour(juce::TextButton::buttonOnColourId, PURPLE.withAlpha(0.25f));
        tabBtns[i].setColour(juce::TextButton::textColourOffId,  juce::Colour(VW::TEXT_MID));
        tabBtns[i].setColour(juce::TextButton::textColourOnId,   PURPLE);
        addAndMakeVisible(tabBtns[i]);
        int idx = i;
        tabBtns[i].onClick = [this, idx] { showLFO(idx); };
    }

    iniKnob(sRate1,  lRate1);   iniKnob(sDepth1, lDepth1);
    iniKnob(sPhase1, lPhase1);  iniKnob(sFade1,  lFade1);
    iniKnob(sSyncDiv1, lSyncDiv1);
    iniKnob(sRate2,  lRate2);   iniKnob(sDepth2, lDepth2);
    iniKnob(sPhase2, lPhase2);  iniKnob(sFade2,  lFade2);
    iniKnob(sSyncDiv2, lSyncDiv2);
    iniCombo(cShape1); iniCombo(cTrig1); iniCombo(cTarget1);
    iniCombo(cShape2); iniCombo(cTrig2); iniCombo(cTarget2);
    for (auto* l : {&lShape1,&lTrig1,&lTarget1,&lShape2,&lTrig2,&lTarget2})
    {
        l->setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.0f, juce::Font::plain));
        l->setColour(juce::Label::textColourId, juce::Colour(VW::TEXT_MID));
        addAndMakeVisible(l);
    }
    for (auto* btn : { &tSync1, &tSync2 })
    {
        btn->setClickingTogglesState(true);
        btn->setColour(juce::TextButton::buttonColourId,   juce::Colour(VW::BG_RAISED));
        btn->setColour(juce::TextButton::buttonOnColourId, PURPLE.withAlpha(0.3f));
        btn->setColour(juce::TextButton::textColourOffId,  juce::Colour(VW::TEXT_MID));
        btn->setColour(juce::TextButton::textColourOnId,   PURPLE);
        addAndMakeVisible(btn);
    }

    auto pop = [&](juce::ComboBox& c, const juce::String& id) {
        if (auto* p = dynamic_cast<juce::AudioParameterChoice*>(t.getParameter(id)))
            c.addItemList(p->choices, 1);
    };
    pop(cShape1, "lfo1_shape"); pop(cTrig1, "lfo1_trigger"); pop(cTarget1, "lfo1_target");
    pop(cShape2, "lfo2_shape"); pop(cTrig2, "lfo2_trigger"); pop(cTarget2, "lfo2_target");

    aRate1  = std::make_unique<SlAtt>(t,"lfo1_rate",       sRate1);
    aDepth1 = std::make_unique<SlAtt>(t,"lfo1_depth",      sDepth1);
    aPhase1 = std::make_unique<SlAtt>(t,"lfo1_phase",      sPhase1);
    aFade1  = std::make_unique<SlAtt>(t,"lfo1_fade",       sFade1);
    aDiv1   = std::make_unique<SlAtt>(t,"lfo1_sync_div",   sSyncDiv1);
    aShape1 = std::make_unique<CbAtt>(t,"lfo1_shape",      cShape1);
    aTrig1  = std::make_unique<CbAtt>(t,"lfo1_trigger",    cTrig1);
    aTarget1= std::make_unique<CbAtt>(t,"lfo1_target",     cTarget1);
    aSync1  = std::make_unique<BtAtt>(t,"lfo1_tempo_sync", tSync1);

    aRate2  = std::make_unique<SlAtt>(t,"lfo2_rate",       sRate2);
    aDepth2 = std::make_unique<SlAtt>(t,"lfo2_depth",      sDepth2);
    aPhase2 = std::make_unique<SlAtt>(t,"lfo2_phase",      sPhase2);
    aFade2  = std::make_unique<SlAtt>(t,"lfo2_fade",       sFade2);
    aDiv2   = std::make_unique<SlAtt>(t,"lfo2_sync_div",   sSyncDiv2);
    aShape2 = std::make_unique<CbAtt>(t,"lfo2_shape",      cShape2);
    aTrig2  = std::make_unique<CbAtt>(t,"lfo2_trigger",    cTrig2);
    aTarget2= std::make_unique<CbAtt>(t,"lfo2_target",     cTarget2);
    aSync2  = std::make_unique<BtAtt>(t,"lfo2_tempo_sync", tSync2);

    sectionTitle.setText("LFO", juce::dontSendNotification);
    sectionTitle.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.0f, juce::Font::bold));
    sectionTitle.setColour(juce::Label::textColourId, PURPLE);
    addAndMakeVisible(sectionTitle);

    pShape1 = t.getRawParameterValue("lfo1_shape");
    pShape2 = t.getRawParameterValue("lfo2_shape");
    pRate1  = t.getRawParameterValue("lfo1_rate");
    pRate2  = t.getRawParameterValue("lfo2_rate");

    sRate1   .setComponentID("lfo1_rate");   sDepth1 .setComponentID("lfo1_depth");
    sPhase1  .setComponentID("lfo1_phase");  sFade1  .setComponentID("lfo1_fade");
    sSyncDiv1.setComponentID("lfo1_sync_div");
    sRate2   .setComponentID("lfo2_rate");   sDepth2 .setComponentID("lfo2_depth");
    sPhase2  .setComponentID("lfo2_phase");  sFade2  .setComponentID("lfo2_fade");
    sSyncDiv2.setComponentID("lfo2_sync_div");

    showLFO(0);
    startTimerHz(30);
}

void LFOSection::timerCallback()
{
    lfoAnimPhase += 0.04f;
    
    bool s1 = tSync1.getToggleState();
    sRate1.setVisible(!s1); lRate1.setVisible(!s1);
    sSyncDiv1.setVisible(s1); lSyncDiv1.setVisible(s1);
    
    bool s2 = tSync2.getToggleState();
    sRate2.setVisible(!s2); lRate2.setVisible(!s2);
    sSyncDiv2.setVisible(s2); lSyncDiv2.setVisible(s2);

    repaint();
}

void LFOSection::showLFO(int idx)
{
    currentLFO = juce::jlimit(0, 1, idx);
    // Tabs only control which LFO is shown in the viz — both rows always visible
    for (int i = 0; i < 2; ++i)
        tabBtns[i].setToggleState(i == currentLFO, juce::dontSendNotification);
    repaint();
}

// ── LFO waveform sample ───────────────────────────────────────────────────────

float LFOSection::lfoSample(int shape, float phase)
{
    float t = phase - std::floor(phase);
    float a = t * juce::MathConstants<float>::twoPi;
    switch (shape)
    {
        case 0:  return std::sin(a);
        case 1:  return 2.0f * t - 1.0f;
        case 2:  return 1.0f - 2.0f * t;
        case 3:  return 2.0f * t - 1.0f;
        case 4:  return t < 0.5f ? 1.0f : -1.0f;
        default: return 0.0f;
    }
}

void LFOSection::drawLFOPreview(juce::Graphics& g, juce::Rectangle<int> bounds) const
{
    auto r = bounds.toFloat();
    auto* pS = currentLFO == 0 ? pShape1 : pShape2;
    auto* pR = currentLFO == 0 ? pRate1  : pRate2;
    int   shape = pS ? static_cast<int>(pS->load()) : 0;
    float rate  = pR ? pR->load() : 1.0f;
    float speed = rate * 0.83f;

    const int N = 128;
    juce::Path path;
    float cy = r.getCentreY(), amp = r.getHeight() * 0.38f;
    for (int i = 0; i < N; ++i)
    {
        float t     = static_cast<float>(i) / static_cast<float>(N - 1);
        float phase = t * 2.0f - lfoAnimPhase * speed;
        float s     = lfoSample(shape, phase);
        float x     = r.getX() + t * r.getWidth();
        float y     = cy - s * amp;
        if (i == 0) path.startNewSubPath(x, y);
        else        path.lineTo(x, y);
    }
    g.setColour(PURPLE.withAlpha(0.12f)); g.strokePath(path, juce::PathStrokeType(4.0f));
    g.setColour(PURPLE.withAlpha(0.25f)); g.strokePath(path, juce::PathStrokeType(2.5f));
    g.setColour(PURPLE.withAlpha(0.85f)); g.strokePath(path, juce::PathStrokeType(1.5f));

    float ph = std::fmod(lfoAnimPhase * speed, 2.0f) / 2.0f;
    float px = r.getX() + ph * r.getWidth();
    g.setColour(PURPLE.withAlpha(0.6f));
    g.drawLine(px, r.getY() + 2, px, r.getBottom() - 2, 1.0f);
}

// ── paint / resized ───────────────────────────────────────────────────────────

void LFOSection::paint(juce::Graphics& g)
{
    const int pad = 6, W = getWidth();

    // Animated LFO preview
    drawLFOPreview(g, juce::Rectangle<int>(pad, 17, W - 2*pad, 30));

    // Routing hint (removed since we have a dedicated dropdown now, keeping spacing)
    // g.drawText(target, pad + 2, 48, W - 2*pad - 4, 9, juce::Justification::right);

    // Row headers — LFO 1 fixed, LFO 2 shifted +10px
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 7.0f, juce::Font::bold));
    g.setColour(PURPLE.withAlpha(0.55f));
    g.drawText("LFO 1", pad, 70, 50, 9, juce::Justification::left);
    g.drawText("LFO 2", pad, 168, 50, 9, juce::Justification::left);
}

void LFOSection::resized()
{
    const int W = getWidth(), pad = 6;

    sectionTitle.setBounds(pad, 1, W - 2*pad, 11);

    // Tabs below preview — only control viz selection
    int tw = (W - 2*pad) / 2;
    tabBtns[0].setBounds(pad,      52, tw, 14);
    tabBtns[1].setBounds(pad+tw+2, 52, tw, 14);

    // ── LFO 1 controls — unchanged position ──────────────────────────────
    {
        int syncW = 28;
        int cw = (W - 4*pad - syncW) / 3;
        int x0 = pad;
        lShape1.setBounds(x0, 87, cw, 10); cShape1.setBounds(x0, 97, cw, 16);
        x0 += cw + pad;
        lTrig1.setBounds(x0, 87, cw, 10); cTrig1.setBounds(x0, 97, cw, 16);
        x0 += cw + pad;
        lTarget1.setBounds(x0, 87, cw, 10); cTarget1.setBounds(x0, 97, cw, 16);
        x0 += cw + pad;
        tSync1.setBounds(W-pad-syncW, 97, syncW, 16);

        const int Km = 42, kw = (W-2*pad)/4;
        juce::Slider* ks[] = {&sRate1,&sDepth1,&sPhase1,&sFade1};
        juce::Label*  ls[] = {&lRate1,&lDepth1,&lPhase1,&lFade1};
        for (int i = 0; i < 4; ++i)
        {
            int x = pad + i*kw + (kw-Km)/2;
            ks[i]->setBounds(x, 117, Km, Km);
            ls[i]->setBounds(pad + i*kw, 117+Km+2, kw, 10);
            if (i == 0) {
                sSyncDiv1.setBounds(x, 117, Km, Km);
                lSyncDiv1.setBounds(pad + i*kw, 117+Km+2, kw, 10);
            }
        }
    }

    // ── LFO 2 controls (always visible) ──────────────────────────────────
    {
        int syncW = 28;
        int cw = (W - 4*pad - syncW) / 3;
        int x0 = pad;
        lShape2.setBounds(x0, 185, cw, 10); cShape2.setBounds(x0, 195, cw, 16);
        x0 += cw + pad;
        lTrig2.setBounds(x0, 185, cw, 10); cTrig2.setBounds(x0, 195, cw, 16);
        x0 += cw + pad;
        lTarget2.setBounds(x0, 185, cw, 10); cTarget2.setBounds(x0, 195, cw, 16);
        x0 += cw + pad;
        tSync2.setBounds(W-pad-syncW, 195, syncW, 16);

        const int Km = 42, kw = (W-2*pad)/4;
        juce::Slider* ks[] = {&sRate2,&sDepth2,&sPhase2,&sFade2};
        juce::Label*  ls[] = {&lRate2,&lDepth2,&lPhase2,&lFade2};
        for (int i = 0; i < 4; ++i)
        {
            int x = pad + i*kw + (kw-Km)/2;
            ks[i]->setBounds(x, 215, Km, Km);
            ls[i]->setBounds(pad + i*kw, 215+Km+2, kw, 10);
            if (i == 0) {
                sSyncDiv2.setBounds(x, 215, Km, Km);
                lSyncDiv2.setBounds(pad + i*kw, 215+Km+2, kw, 10);
            }
        }
    }
}
