#include "LFOSection.h"
#include "LookAndFeel.h"
static const juce::Colour PURPLE { VW::NEON_AMBER };

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
    iniKnob(sSyncDiv1,lSyncDiv1);
    iniKnob(sRate2,  lRate2);   iniKnob(sDepth2, lDepth2);
    iniKnob(sPhase2, lPhase2);  iniKnob(sFade2,  lFade2);
    iniKnob(sSyncDiv2,lSyncDiv2);
    iniCombo(cShape1); iniCombo(cTrig1);
    iniCombo(cShape2); iniCombo(cTrig2);
    for (auto* l : {&lShape1,&lTrig1,&lShape2,&lTrig2})
    {
        l->setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.0f, juce::Font::plain));
        l->setColour(juce::Label::textColourId, juce::Colour(VW::TEXT_MID));
        addAndMakeVisible(l);
    }

    // Tempo sync toggle buttons
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
    pop(cShape1, "lfo1_shape"); pop(cTrig1, "lfo1_trigger");
    pop(cShape2, "lfo2_shape"); pop(cTrig2, "lfo2_trigger");

    aRate1   = std::make_unique<SlAtt>(t,"lfo1_rate",       sRate1);
    aDepth1  = std::make_unique<SlAtt>(t,"lfo1_depth",      sDepth1);
    aPhase1  = std::make_unique<SlAtt>(t,"lfo1_phase",      sPhase1);
    aFade1   = std::make_unique<SlAtt>(t,"lfo1_fade",       sFade1);
    aDiv1    = std::make_unique<SlAtt>(t,"lfo1_sync_div",   sSyncDiv1);
    aShape1  = std::make_unique<CbAtt>(t,"lfo1_shape",      cShape1);
    aTrig1   = std::make_unique<CbAtt>(t,"lfo1_trigger",    cTrig1);
    aSync1   = std::make_unique<BtAtt>(t,"lfo1_tempo_sync", tSync1);

    aRate2   = std::make_unique<SlAtt>(t,"lfo2_rate",       sRate2);
    aDepth2  = std::make_unique<SlAtt>(t,"lfo2_depth",      sDepth2);
    aPhase2  = std::make_unique<SlAtt>(t,"lfo2_phase",      sPhase2);
    aFade2   = std::make_unique<SlAtt>(t,"lfo2_fade",       sFade2);
    aDiv2    = std::make_unique<SlAtt>(t,"lfo2_sync_div",   sSyncDiv2);
    aShape2  = std::make_unique<CbAtt>(t,"lfo2_shape",      cShape2);
    aTrig2   = std::make_unique<CbAtt>(t,"lfo2_trigger",    cTrig2);
    aSync2   = std::make_unique<BtAtt>(t,"lfo2_tempo_sync", tSync2);

    // Cache raw params for preview
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

void LFOSection::showLFO(int idx)
{
    currentLFO = idx;
    auto sv = [](std::initializer_list<juce::Component*> cs, bool v)
    { for (auto* c : cs) c->setVisible(v); };
    sv({&sRate1,&sDepth1,&sPhase1,&sFade1,&sSyncDiv1,&lRate1,&lDepth1,&lPhase1,&lFade1,&lSyncDiv1,&cShape1,&lShape1,&cTrig1,&lTrig1,&tSync1}, idx==0);
    sv({&sRate2,&sDepth2,&sPhase2,&sFade2,&sSyncDiv2,&lRate2,&lDepth2,&lPhase2,&lFade2,&lSyncDiv2,&cShape2,&lShape2,&cTrig2,&lTrig2,&tSync2}, idx==1);
    for (int i=0;i<2;++i) tabBtns[i].setToggleState(i==idx, juce::dontSendNotification);
    resized(); repaint();
}

// ── LFO waveform sample (0-1 phase → -1..1) ──────────────────────────────────

float LFOSection::lfoSample(int shape, float phase)
{
    float t = phase - std::floor(phase);   // wrap to 0-1
    float a = t * juce::MathConstants<float>::twoPi;
    switch (shape)
    {
        case 0:  return std::sin(a);                                 // Sine
        case 1:  return 2.0f * t - 1.0f;                            // Tri (approx)
        case 2:  return 1.0f - 2.0f * t;                            // Saw
        case 3:  return 2.0f * t - 1.0f;                            // Ramp
        case 4:  return t < 0.5f ? 1.0f : -1.0f;                    // Square
        default: return 0.0f;
    }
}

void LFOSection::drawLFOPreview(juce::Graphics& g, juce::Rectangle<int> bounds) const
{
    auto r = bounds.toFloat();
    g.setColour(juce::Colour(VW::BG_PANEL));
    g.fillRoundedRectangle(r, 3.0f);
    g.setColour(juce::Colour(VW::BORDER_VIS));
    g.drawRoundedRectangle(r, 3.0f, 1.0f);

    int shape = 0;
    auto* pS = currentLFO == 0 ? pShape1 : pShape2;
    auto* pR = currentLFO == 0 ? pRate1  : pRate2;
    if (pS) shape = static_cast<int>(pS->load());
    float rate = pR ? pR->load() : 1.0f;

    const int N = 128;
    juce::Path path;
    float cy = r.getCentreY(), amp = r.getHeight() * 0.38f;
    // Speed scales with rate: 1Hz ≈ 1 visible cycle per second at 30fps
    // lfoAnimPhase increments 0.04/frame → 30*0.04=1.2/sec. 1 cycle = 2.0 units → speed = rate/1.2*2.0
    float speed = rate * 0.83f;

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

    // Glow
    g.setColour(PURPLE.withAlpha(0.12f));
    g.strokePath(path, juce::PathStrokeType(4.0f));
    g.setColour(PURPLE.withAlpha(0.25f));
    g.strokePath(path, juce::PathStrokeType(2.5f));
    g.setColour(PURPLE.withAlpha(0.85f));
    g.strokePath(path, juce::PathStrokeType(1.5f));

    // Scrolling playhead
    float ph = std::fmod(lfoAnimPhase * speed, 2.0f) / 2.0f;
    float px = r.getX() + ph * r.getWidth();
    g.setColour(PURPLE.withAlpha(0.6f));
    g.drawLine(px, r.getY() + 2, px, r.getBottom() - 2, 1.0f);
}

// ── paint / resized ───────────────────────────────────────────────────────────

void LFOSection::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(VW::BG_PANEL));
    g.setColour(PURPLE);
    g.fillRect(0, 0, getWidth(), 14);
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xffffffff));
    g.drawText("LFO", 0, 0, getWidth(), 14, juce::Justification::centred);
    g.setColour(juce::Colour(VW::BORDER_SUB));
    g.drawRect(getLocalBounds(), 1);

    // Animated LFO preview canvas
    const int pad = 6;
    drawLFOPreview(g, juce::Rectangle<int>(pad, 36, getWidth() - 2*pad, 30));

    // Default routing hint — bottom-right of preview area
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 7.0f, juce::Font::plain));
    g.setColour(PURPLE.withAlpha(0.45f));
    juce::String target = (currentLFO == 0) ? "-> WT POS" : "-> FILTER";
    g.drawText(target, pad + 2, 57, getWidth() - 2*pad - 4, 9, juce::Justification::right);
}

void LFOSection::resized()
{
    const int W = getWidth(), pad = 6;
    int tw = (W - 2*pad) / 2;
    tabBtns[0].setBounds(pad,        16, tw, 14);
    tabBtns[1].setBounds(pad+tw+2,   16, tw, 14);

    // Preview occupies y=36..66 (drawn in paint), controls start at y=70
    int cw = (W - 3*pad) / 2;
    auto& cS = currentLFO==0 ? cShape1 : cShape2;
    auto& lS = currentLFO==0 ? lShape1 : lShape2;
    auto& cT = currentLFO==0 ? cTrig1  : cTrig2;
    auto& lT = currentLFO==0 ? lTrig1  : lTrig2;
    auto& tS = currentLFO==0 ? tSync1  : tSync2;
    lS.setBounds(pad,          70, cw, 10);      cS.setBounds(pad,          80, cw, 16);
    lT.setBounds(pad+cw+pad,   70, cw - 30, 10); cT.setBounds(pad+cw+pad,   80, cw - 32, 16);
    tS.setBounds(W - pad - 28, 80, 28, 16);

    // 5 mini knobs
    const int Km = 28;
    juce::Slider* ks[] = { currentLFO==0?&sRate1:&sRate2,    currentLFO==0?&sDepth1:&sDepth2,
                            currentLFO==0?&sPhase1:&sPhase2,  currentLFO==0?&sFade1:&sFade2,
                            currentLFO==0?&sSyncDiv1:&sSyncDiv2 };
    juce::Label*  ls[] = { currentLFO==0?&lRate1:&lRate2,    currentLFO==0?&lDepth1:&lDepth2,
                            currentLFO==0?&lPhase1:&lPhase2,  currentLFO==0?&lFade1:&lFade2,
                            currentLFO==0?&lSyncDiv1:&lSyncDiv2 };
    int kw = (W - 2*pad) / 5;
    for (int i = 0; i < 5; ++i)
    {
        int x = pad + i*kw + (kw-Km)/2;
        ks[i]->setBounds(x, 100, Km, Km);
        ls[i]->setBounds(pad + i*kw, 100+Km+2, kw, 10);
    }
}
