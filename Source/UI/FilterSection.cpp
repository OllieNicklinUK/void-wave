#include "FilterSection.h"
#include "LookAndFeel.h"
static const juce::Colour AMBER { VW::SEC_FILTER };

// ── Shared ADSR drawing ───────────────────────────────────────────────────────

void FilterSection::drawADSR(juce::Graphics& g, juce::Rectangle<float> area,
                              float atk, float hld, float dcy, float sus, float rel,
                              juce::Colour col)
{
    float sustain_width = 0.15f;  // fixed sustain display fraction of total
    float total = atk + hld + dcy + sustain_width + rel;
    if (total < 0.001f) return;

    float x0 = area.getX(), w = area.getWidth(), h = area.getHeight();
    float yb = area.getBottom() - 1.0f;   // baseline
    float yt = area.getY()     + 2.0f;    // peak
    float ys = yt + (1.0f - juce::jlimit(0.0f, 1.0f, sus)) * (yb - yt);

    auto tx = [&](float t) { return x0 + (t / total) * w; };

    float t1 = atk;
    float t2 = atk + hld;
    float t3 = atk + hld + dcy;
    float t4 = t3 + sustain_width;
    float t5 = total;

    juce::Path path;
    path.startNewSubPath(x0,     yb);
    path.lineTo(tx(t1), yt);
    path.lineTo(tx(t2), yt);
    path.lineTo(tx(t3), ys);
    path.lineTo(tx(t4), ys);
    path.lineTo(tx(t5), yb);

    // Gradient fill
    juce::ColourGradient grad { col.withAlpha(0.18f), x0, yt, col.withAlpha(0.0f), x0, yb, false };
    juce::Path fill = path;
    fill.lineTo(tx(t5), yb);
    fill.lineTo(x0,     yb);
    fill.closeSubPath();
    g.setGradientFill(grad);
    g.fillPath(fill);

    // Outer glow
    g.setColour(col.withAlpha(0.2f));
    g.strokePath(path, juce::PathStrokeType(3.0f));
    // Main stroke
    g.setColour(col.withAlpha(0.85f));
    g.strokePath(path, juce::PathStrokeType(1.5f));

    // Stage tick marks
    g.setColour(col.withAlpha(0.35f));
    for (float tx_ : { tx(t1), tx(t2), tx(t3), tx(t4) })
        g.drawVerticalLine(static_cast<int>(tx_), yt, yb);
}

// ── FilterSection ─────────────────────────────────────────────────────────────

FilterSection::FilterSection(VoidWaveAudioProcessor& p) : processor(p)
{
    auto& t = p.apvts;
    auto sk = [&](juce::Slider& s, juce::Label& l) {
        s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        s.setColour(juce::Slider::thumbColourId,               AMBER);
        s.setColour(juce::Slider::rotarySliderFillColourId,    AMBER);
        s.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(VW::BORDER_VIS));
        addAndMakeVisible(s);
        l.setJustificationType(juce::Justification::centred);
        l.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.0f, juce::Font::plain));
        l.setColour(juce::Label::textColourId, juce::Colour(VW::TEXT_MID));
        addAndMakeVisible(l);
    };
    sk(sCutoff, lCutoff); sk(sRes, lRes); sk(sDrive, lDrive);
    sk(sKeyTrack, lKey);  sk(sVelTrack, lVel);
    sk(sEnvDepth, lEnvDepth);
    sEnvDepth.setComponentID("env1_depth");

    // Cutoff: make it look bigger (large arc)
    sCutoff.setSkewFactorFromMidPoint(1000.0);

    cType.setColour(juce::ComboBox::backgroundColourId, juce::Colour(VW::BG_RAISED));
    cType.setColour(juce::ComboBox::textColourId,       juce::Colour(VW::TEXT_BRIGHT));
    cType.setColour(juce::ComboBox::outlineColourId,    juce::Colour(VW::BORDER_VIS));
    cType.setColour(juce::ComboBox::arrowColourId,      AMBER);
    if (auto* cp = dynamic_cast<juce::AudioParameterChoice*>(t.getParameter("filter_type")))
        cType.addItemList(cp->choices, 1);
    addAndMakeVisible(cType);

    lType.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.0f, juce::Font::plain));
    lType.setColour(juce::Label::textColourId, juce::Colour(VW::TEXT_MID));
    addAndMakeVisible(lType);

    attCutoff   = std::make_unique<SlAtt>(t, "filter_cutoff",   sCutoff);
    attRes      = std::make_unique<SlAtt>(t, "filter_res",      sRes);
    attDrive    = std::make_unique<SlAtt>(t, "filter_drive",    sDrive);
    attKey      = std::make_unique<SlAtt>(t, "filter_keytrack", sKeyTrack);
    attVel      = std::make_unique<SlAtt>(t, "filter_veltrack", sVelTrack);
    attType     = std::make_unique<CbAtt>(t, "filter_type",     cType);
    attEnvDepth = std::make_unique<SlAtt>(t, "env1_depth",      sEnvDepth);

    // OSC route buttons
    static const char* routeLabels[] = { "1+2", "1", "2" };
    pFilterRoute = t.getRawParameterValue("filter_route");
    for (int i = 0; i < 3; ++i)
    {
        btnRoute[i].setButtonText(routeLabels[i]);
        btnRoute[i].setClickingTogglesState(false);
        btnRoute[i].onClick = [this, i] { setRoute(i); };
        btnRoute[i].getProperties().set("noHover", true);
        addAndMakeVisible(btnRoute[i]);
    }
    updateRouteButtons();

    // Section title
    sectionTitle.setText("FILTER", juce::dontSendNotification);
    sectionTitle.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.0f, juce::Font::bold));
    sectionTitle.setColour(juce::Label::textColourId, AMBER);
    addAndMakeVisible(sectionTitle);

    // ENV1 pointers for live ADSR viz
    pEnv1Atk = t.getRawParameterValue("env1_attack");
    pEnv1Dcy = t.getRawParameterValue("env1_decay");
    pEnv1Sus = t.getRawParameterValue("env1_sustain");
    pEnv1Hld = t.getRawParameterValue("env1_hold");
    pEnv1Rel = t.getRawParameterValue("env1_release");
    pEnv1Dep = t.getRawParameterValue("env1_depth");

    sCutoff  .setComponentID("filter_cutoff");
    sRes     .setComponentID("filter_res");
    sDrive   .setComponentID("filter_drive");
    sKeyTrack.setComponentID("filter_keytrack");
    sVelTrack.setComponentID("filter_veltrack");

    startTimerHz(30);
}

void FilterSection::setRoute(int route)
{
    if (auto* param = dynamic_cast<juce::AudioParameterInt*>(
            processor.apvts.getParameter("filter_route")))
        param->setValueNotifyingHost(param->getNormalisableRange().convertTo0to1(float(route)));
    updateRouteButtons();
}

void FilterSection::updateRouteButtons()
{
    int cur = pFilterRoute ? static_cast<int>(pFilterRoute->load()) : 0;
    for (int i = 0; i < 3; ++i)
    {
        bool on = (i == cur);
        btnRoute[i].setColour(juce::TextButton::buttonColourId,
            on ? AMBER.withAlpha(0.85f) : juce::Colour(VW::BG_CONTROL));
        btnRoute[i].setColour(juce::TextButton::textColourOnId,
            on ? juce::Colour(VW::BG_RAISED) : AMBER);
        btnRoute[i].setColour(juce::TextButton::textColourOffId,
            on ? juce::Colour(VW::BG_RAISED) : AMBER.withAlpha(0.5f));
        btnRoute[i].repaint();
    }
}

void FilterSection::paint(juce::Graphics& g)
{
    const int W = getWidth(), H = getHeight(), pad = 8;

    // ADSR visualisation canvas (no fill — PNG background shows through)
    auto vizArea = juce::Rectangle<float>(pad, 27, W - 2*pad, 68);

    // Grid
    g.setColour(AMBER.withAlpha(0.04f));
    g.drawHorizontalLine(static_cast<int>(vizArea.getCentreY()), vizArea.getX()+2, vizArea.getRight()-2);

    // Depth indicator — vertical dashed line showing ENV1 depth range
    if (pEnv1Dep)
    {
        float dep  = pEnv1Dep->load();
        float depH = vizArea.getHeight() * std::abs(dep) * 0.4f;
        g.setColour(AMBER.withAlpha(0.15f));
        g.fillRect(juce::Rectangle<float>(vizArea.getX(), vizArea.getCentreY() - depH,
                                          3.0f, depH * 2.0f));
    }

    // Draw ENV1 ADSR curve
    if (pEnv1Atk && pEnv1Dcy && pEnv1Sus && pEnv1Hld && pEnv1Rel)
        drawADSR(g, vizArea.reduced(4, 4),
                 pEnv1Atk->load(), pEnv1Hld->load(), pEnv1Dcy->load(),
                 pEnv1Sus->load(), pEnv1Rel->load(), AMBER);

    juce::ignoreUnused(H);
}

void FilterSection::resized()
{
    const int W = getWidth(), pad = 8;

    // Section title
    sectionTitle.setBounds(pad, 1, W - 2*pad, 11);

    // OSC route buttons — right side of TYPE row, same height as combo
    const int btnH = 17, btnW = 26, btnGap = 2;
    int btnGroupW = 3 * btnW + 2 * btnGap;
    int btnX = W - pad - btnGroupW;
    for (int i = 0; i < 3; ++i)
        btnRoute[i].setBounds(btnX + i * (btnW + btnGap), 102, btnW, btnH);

    // TYPE combo — aligned with route buttons
    lType.setBounds(pad, 104, 30, 10);
    cType.setBounds(pad + 32, 102, btnX - pad - 32 - 6, 17);

    // 3-column primary: CUTOFF (larger) | RES | ENV DEP
    // 3-column secondary: DRIVE | KEY TRK | VEL TRK
    const int KC = 56, KR = 46, Km = 36;
    const int H  = getHeight();
    const int kw = (W - 2*pad) / 3;   // equal thirds

    const int primaryH   = KC + 14;
    const int secondaryH = Km + 14;
    const int avail      = H - 116;
    const int totalUsed  = primaryH + secondaryH + 24;
    const int topPad     = (avail - totalUsed) / 2;
    const int kY  = 114 + topPad;
    const int kY2 = kY + primaryH + 24;

    auto plK = [&](juce::Slider& s, juce::Label& l, int col, int size, int row)
    {
        int x   = pad + col*kw + (kw - size) / 2;
        int vc  = (row == 0) ? kY + KC/2 : kY2;
        int top = vc - size/2;
        s.setBounds(x, top, size, size);
        l.setBounds(pad + col*kw, top + size + 3, kw, 10);  // 3px below knob bottom for both rows
    };

    plK(sCutoff,   lCutoff,   0, KC, 0);
    plK(sRes,      lRes,      1, KR, 0);
    plK(sEnvDepth, lEnvDepth, 2, KR, 0);
    plK(sDrive,    lDrive,    0, Km, 1);
    plK(sKeyTrack, lKey,      1, Km, 1);
    plK(sVelTrack, lVel,      2, Km, 1);
}
