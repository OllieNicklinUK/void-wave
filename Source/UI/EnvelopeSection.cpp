#include "EnvelopeSection.h"
#include "LookAndFeel.h"

static const juce::Colour GREEN { VW::NEON_AMBER };

void EnvelopeSection::iniKnob(juce::Slider& s, juce::Label& l)
{
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    s.setColour(juce::Slider::thumbColourId,               GREEN);
    s.setColour(juce::Slider::rotarySliderFillColourId,    GREEN);
    s.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(VW::BORDER_VIS));
    addAndMakeVisible(s);
    l.setJustificationType(juce::Justification::centred);
    l.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.0f, juce::Font::plain));
    l.setColour(juce::Label::textColourId, juce::Colour(VW::TEXT_MID));
    addAndMakeVisible(l);
}
void EnvelopeSection::iniCombo(juce::ComboBox& c)
{
    c.setColour(juce::ComboBox::backgroundColourId, juce::Colour(VW::BG_RAISED));
    c.setColour(juce::ComboBox::textColourId,       juce::Colour(VW::TEXT_BRIGHT));
    c.setColour(juce::ComboBox::outlineColourId,    juce::Colour(VW::BORDER_VIS));
    c.setColour(juce::ComboBox::arrowColourId,      GREEN);
    addAndMakeVisible(c);
}

EnvelopeSection::EnvelopeSection(VoidWaveAudioProcessor& p) : processor(p)
{
    auto& t = p.apvts;

    // Two tabs: FILTER ENV (env1) and AMP ENV (env2). ENV 3 hidden — still in APVTS.
    const char* labels[] = { "FILTER ENV", "AMP ENV", "" };
    for (int i = 0; i < 2; ++i)
    {
        tabBtns[i].setButtonText(labels[i]);
        tabBtns[i].setClickingTogglesState(false);
        tabBtns[i].setColour(juce::TextButton::buttonColourId,   juce::Colour(VW::BG_RAISED));
        tabBtns[i].setColour(juce::TextButton::buttonOnColourId, GREEN.withAlpha(0.25f));
        tabBtns[i].setColour(juce::TextButton::textColourOffId,  juce::Colour(VW::TEXT_MID));
        tabBtns[i].setColour(juce::TextButton::textColourOnId,   GREEN);
        addAndMakeVisible(tabBtns[i]);
        int idx = i;
        tabBtns[i].onClick = [this, idx] { showEnv(idx); };
    }
    tabBtns[2].setVisible(false);  // ENV 3 tab hidden

    iniKnob(sAtk1,lAtk1); iniKnob(sDcy1,lDcy1); iniKnob(sSus1,lSus1);
    iniKnob(sHld1,lHld1); iniKnob(sRel1,lRel1); iniKnob(sDep1,lDep1);
    iniKnob(sAtk2,lAtk2); iniKnob(sDcy2,lDcy2); iniKnob(sSus2,lSus2);
    iniKnob(sHld2,lHld2); iniKnob(sRel2,lRel2);
    iniKnob(sAtk3,lAtk3); iniKnob(sDcy3,lDcy3); iniKnob(sSus3,lSus3);
    iniKnob(sHld3,lHld3); iniKnob(sRel3,lRel3);
    iniCombo(cCrv1); addAndMakeVisible(lCrv1); lCrv1.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.0f, juce::Font::plain));
    iniCombo(cCrv2); addAndMakeVisible(lCrv2); lCrv2.setFont(lCrv1.getFont());
    iniCombo(cCrv3); addAndMakeVisible(lCrv3); lCrv3.setFont(lCrv1.getFont());

    auto pop = [&](juce::ComboBox& c, const juce::String& id) {
        if (auto* p = dynamic_cast<juce::AudioParameterChoice*>(t.getParameter(id)))
            c.addItemList(p->choices, 1);
    };
    pop(cCrv1, "env1_curve"); pop(cCrv2, "env2_curve"); pop(cCrv3, "env3_curve");

    aAtk1 = std::make_unique<SlAtt>(t,"env1_attack",  sAtk1); aDcy1 = std::make_unique<SlAtt>(t,"env1_decay",   sDcy1);
    aSus1 = std::make_unique<SlAtt>(t,"env1_sustain", sSus1); aHld1 = std::make_unique<SlAtt>(t,"env1_hold",    sHld1);
    aRel1 = std::make_unique<SlAtt>(t,"env1_release", sRel1); aDep1 = std::make_unique<SlAtt>(t,"env1_depth",   sDep1);
    aCrv1 = std::make_unique<CbAtt>(t,"env1_curve",   cCrv1);

    aAtk2 = std::make_unique<SlAtt>(t,"env2_attack",  sAtk2); aDcy2 = std::make_unique<SlAtt>(t,"env2_decay",   sDcy2);
    aSus2 = std::make_unique<SlAtt>(t,"env2_sustain", sSus2); aHld2 = std::make_unique<SlAtt>(t,"env2_hold",    sHld2);
    aRel2 = std::make_unique<SlAtt>(t,"env2_release", sRel2);
    aCrv2 = std::make_unique<CbAtt>(t,"env2_curve",   cCrv2);

    aAtk3 = std::make_unique<SlAtt>(t,"env3_attack",  sAtk3); aDcy3 = std::make_unique<SlAtt>(t,"env3_decay",   sDcy3);
    aSus3 = std::make_unique<SlAtt>(t,"env3_sustain", sSus3); aHld3 = std::make_unique<SlAtt>(t,"env3_hold",    sHld3);
    aRel3 = std::make_unique<SlAtt>(t,"env3_release", sRel3);
    aCrv3 = std::make_unique<CbAtt>(t,"env3_curve",   cCrv3);

    // Raw pointers for ADSR viz  [envIdx][atk,dcy,sus,hld,rel,dep]
    const char* ids[3][6] = {
        {"env1_attack","env1_decay","env1_sustain","env1_hold","env1_release","env1_depth"},
        {"env2_attack","env2_decay","env2_sustain","env2_hold","env2_release",nullptr},
        {"env3_attack","env3_decay","env3_sustain","env3_hold","env3_release",nullptr},
    };
    for (int e = 0; e < 3; ++e)
        for (int k = 0; k < 6; ++k)
            envPtrs[e][k] = ids[e][k] ? t.getRawParameterValue(ids[e][k]) : nullptr;

    // ComponentIDs for MIDI learn
    sAtk1.setComponentID("env1_attack");  sDcy1.setComponentID("env1_decay");
    sSus1.setComponentID("env1_sustain"); sHld1.setComponentID("env1_hold");
    sRel1.setComponentID("env1_release"); sDep1.setComponentID("env1_depth");
    sAtk2.setComponentID("env2_attack");  sDcy2.setComponentID("env2_decay");
    sSus2.setComponentID("env2_sustain"); sHld2.setComponentID("env2_hold");
    sRel2.setComponentID("env2_release");
    sAtk3.setComponentID("env3_attack");  sDcy3.setComponentID("env3_decay");
    sSus3.setComponentID("env3_sustain"); sHld3.setComponentID("env3_hold");
    sRel3.setComponentID("env3_release");

    showEnv(0);
    startTimerHz(30);
}

void EnvelopeSection::showEnv(int idx)
{
    currentEnv = idx;
    auto setVis = [](std::initializer_list<juce::Component*> comps, bool v)
    { for (auto* c : comps) c->setVisible(v); };

    // Clamp to 2 visible envs — ENV 3 controls always hidden
    idx = juce::jlimit(0, 1, idx);
    setVis({&sAtk1,&sDcy1,&sSus1,&sHld1,&sRel1,&sDep1,&lAtk1,&lDcy1,&lSus1,&lHld1,&lRel1,&lDep1,&cCrv1,&lCrv1}, idx==0);
    setVis({&sAtk2,&sDcy2,&sSus2,&sHld2,&sRel2,&lAtk2,&lDcy2,&lSus2,&lHld2,&lRel2,&cCrv2,&lCrv2}, idx==1);
    // ENV 3 always hidden
    setVis({&sAtk3,&sDcy3,&sSus3,&sHld3,&sRel3,&lAtk3,&lDcy3,&lSus3,&lHld3,&lRel3,&cCrv3,&lCrv3}, false);
    for (int i = 0; i < 2; ++i) tabBtns[i].setToggleState(i == idx, juce::dontSendNotification);
    resized();
    repaint();
}

// ── ADSR drawing (same logic as FilterSection, duplicated for independence) ────

void EnvelopeSection::drawADSR(juce::Graphics& g, juce::Rectangle<float> area,
                                 float atk, float hld, float dcy, float sus, float rel,
                                 juce::Colour col)
{
    float total = atk + hld + dcy + 0.15f + rel;
    if (total < 0.001f) return;
    float x0 = area.getX(), w = area.getWidth(), h = area.getHeight();
    float yb = area.getBottom() - 1.0f, yt = area.getY() + 2.0f;
    float ys = yt + (1.0f - juce::jlimit(0.0f, 1.0f, sus)) * (yb - yt);
    auto tx = [&](float t) { return x0 + (t / total) * w; };
    float t1 = atk, t2 = atk+hld, t3 = t2+dcy, t4 = t3+0.15f, t5 = total;

    juce::Path path;
    path.startNewSubPath(x0,     yb);
    path.lineTo(tx(t1), yt); path.lineTo(tx(t2), yt);
    path.lineTo(tx(t3), ys); path.lineTo(tx(t4), ys);
    path.lineTo(tx(t5), yb);

    juce::ColourGradient grad { col.withAlpha(0.18f), x0, yt, col.withAlpha(0.0f), x0, yb, false };
    juce::Path fill = path;
    fill.lineTo(tx(t5), yb); fill.lineTo(x0, yb); fill.closeSubPath();
    g.setGradientFill(grad);  g.fillPath(fill);
    g.setColour(col.withAlpha(0.2f));  g.strokePath(path, juce::PathStrokeType(3.0f));
    g.setColour(col.withAlpha(0.85f)); g.strokePath(path, juce::PathStrokeType(1.5f));

    // Stage ticks
    g.setColour(col.withAlpha(0.3f));
    for (float tx_ : { tx(t1), tx(t2), tx(t3), tx(t4) })
        g.drawVerticalLine(static_cast<int>(tx_), yt + 2, yb - 2);
}

void EnvelopeSection::paint(juce::Graphics& g)
{
    const int W = getWidth(), pad = 6;
    g.fillAll(juce::Colour(VW::BG_PANEL));
    g.setColour(GREEN);
    g.fillRect(0, 0, W, 14);
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xffffffff));
    // Show which envelope is active in the header
    const char* envName = (currentEnv == 0) ? "ENVELOPE  ·  FILTER" : "ENVELOPE  ·  AMP";
    g.drawText(envName, 0, 0, W, 14, juce::Justification::centred);
    g.setColour(juce::Colour(VW::BORDER_SUB));
    g.drawRect(getLocalBounds(), 1);

    // ADSR visualisation canvas
    auto vizArea = juce::Rectangle<float>(pad, 38, W - 2*pad, 80);
    g.setColour(juce::Colour(VW::BG_PANEL));
    g.fillRoundedRectangle(vizArea, 3.0f);
    g.setColour(juce::Colour(VW::BORDER_VIS));
    g.drawRoundedRectangle(vizArea, 3.0f, 1.0f);

    // Grid
    g.setColour(GREEN.withAlpha(0.04f));
    g.drawHorizontalLine(static_cast<int>(vizArea.getCentreY()), vizArea.getX()+2, vizArea.getRight()-2);

    // Draw ADSR for current envelope tab
    int e = currentEnv;
    if (envPtrs[e][0] != nullptr)
    {
        float atk = envPtrs[e][0]->load(), dcy = envPtrs[e][1]->load();
        float sus = envPtrs[e][2]->load(), hld = envPtrs[e][3]->load();
        float rel = envPtrs[e][4]->load();

        // Filter env: colour by depth polarity — amber=positive, cyan=negative
        juce::Colour vizCol = GREEN;
        if (e == 0 && envPtrs[e][5] != nullptr)
        {
            float depth = envPtrs[e][5]->load();
            if (depth < 0.0f)
                vizCol = juce::Colour(VW::NEON_CYAN);

            // +/- polarity badge, top-right of viz area
            g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 9.0f, juce::Font::bold));
            g.setColour(vizCol.withAlpha(0.9f));
            g.drawText(depth >= 0.0f ? "+" : "-",
                       static_cast<int>(vizArea.getRight()) - 14,
                       static_cast<int>(vizArea.getY()) + 3, 12, 12,
                       juce::Justification::centred);
        }
        drawADSR(g, vizArea.reduced(4, 4), atk, hld, dcy, sus, rel, vizCol);
    }
}

void EnvelopeSection::layoutEnv(int envIdx, juce::Rectangle<int> area)
{
    const int pad = 6, W = area.getWidth(), x0 = area.getX(), y0 = area.getY();
    int numKnobs = (envIdx == 0) ? 6 : 5;
    int kw = (W - 2 * pad) / numKnobs;
    const int Km = 28;

    juce::Slider* knobs[] = { envIdx==0?&sAtk1:envIdx==1?&sAtk2:&sAtk3,
                               envIdx==0?&sDcy1:envIdx==1?&sDcy2:&sDcy3,
                               envIdx==0?&sSus1:envIdx==1?&sSus2:&sSus3,
                               envIdx==0?&sHld1:envIdx==1?&sHld2:&sHld3,
                               envIdx==0?&sRel1:envIdx==1?&sRel2:&sRel3,
                               envIdx==0?&sDep1:nullptr };
    juce::Label* lbls[] =  { envIdx==0?&lAtk1:envIdx==1?&lAtk2:&lAtk3,
                               envIdx==0?&lDcy1:envIdx==1?&lDcy2:&lDcy3,
                               envIdx==0?&lSus1:envIdx==1?&lSus2:&lSus3,
                               envIdx==0?&lHld1:envIdx==1?&lHld2:&lHld3,
                               envIdx==0?&lRel1:envIdx==1?&lRel2:&lRel3,
                               envIdx==0?&lDep1:nullptr };
    juce::ComboBox* crv    = envIdx==0?&cCrv1:envIdx==1?&cCrv2:&cCrv3;
    juce::Label*    crvLbl = envIdx==0?&lCrv1:envIdx==1?&lCrv2:&lCrv3;

    for (int i = 0; i < numKnobs; ++i)
    {
        if (!knobs[i]) continue;
        int x = x0 + pad + i * kw + (kw - Km) / 2;
        knobs[i]->setBounds(x, y0 + 4, Km, Km);
        lbls  [i]->setBounds(x0 + pad + i * kw, y0 + 4 + Km + 2, kw, 10);
    }
    crvLbl->setBounds(x0 + pad, y0 + Km + 20, 40, 10);
    crv   ->setBounds(x0 + pad + 42, y0 + Km + 18, W - pad * 2 - 42, 16);
}

void EnvelopeSection::resized()
{
    const int W = getWidth(), pad = 6;
    // Two tabs — each takes half the width
    int tw = (W - 3 * pad) / 2;
    tabBtns[0].setBounds(pad,          16, tw, 14);
    tabBtns[1].setBounds(pad + tw + pad, 16, tw, 14);

    // Knobs below ADSR viz (viz occupies y=38-118)
    auto area = getLocalBounds().withTrimmedTop(120).reduced(0, 2);
    layoutEnv(currentEnv, area);
}
