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

    attCutoff = std::make_unique<SlAtt>(t, "filter_cutoff",   sCutoff);
    attRes    = std::make_unique<SlAtt>(t, "filter_res",      sRes);
    attDrive  = std::make_unique<SlAtt>(t, "filter_drive",    sDrive);
    attKey    = std::make_unique<SlAtt>(t, "filter_keytrack", sKeyTrack);
    attVel    = std::make_unique<SlAtt>(t, "filter_veltrack", sVelTrack);
    attType   = std::make_unique<CbAtt>(t, "filter_type",     cType);

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

void FilterSection::paint(juce::Graphics& g)
{
    const int W = getWidth(), H = getHeight(), pad = 8;

    // ADSR visualisation canvas (no fill — PNG background shows through)
    auto vizArea = juce::Rectangle<float>(pad, 17, W - 2*pad, 68);

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

    // TYPE combo — nudged down to clear the viz background
    lType.setBounds(pad, 94, 30, 10);
    cType.setBounds(pad + 32, 92, W - pad - 32 - pad, 17);

    // Available vertical space below combo: getHeight() - 104
    // Primary knobs (CUTOFF 50% bigger, RES 30% bigger), secondary 30% bigger
    // Spread to fill the section height
    const int KC  = 63;   // CUTOFF: 42 * 1.5
    const int KR  = 55;   // RES:    42 * 1.3
    const int Km  = 36;   // DRIVE/KEY/VEL: 28 * 1.3
    const int H   = getHeight();
    const int avail = H - 106;                        // space below combo

    // Three zones: primary row, gap, secondary row, gap
    const int primaryH   = KC + 14;                   // knob + label
    const int secondaryH = Km + 14;
    const int totalUsed  = primaryH + secondaryH + 24; // 24px gap between rows
    const int topPad     = (avail - totalUsed) / 2;

    int kY = 104 + topPad;

    int halfW  = (W - 3 * pad) / 2;
    int leftX  = pad + (halfW - KC) / 2;
    int rightX = pad + halfW + pad + (halfW - KR) / 2;
    int centreY = kY + KC / 2;                         // vertical centre of primary row

    sCutoff.setBounds(leftX,              kY,                   KC, KC);
    lCutoff.setBounds(pad,                kY + KC + 3,          halfW, 10);
    sRes   .setBounds(rightX,             centreY - KR / 2,     KR, KR);
    lRes   .setBounds(pad + halfW + pad,  kY + KC + 3,          halfW, 10);

    int kY2  = kY + primaryH + 24;
    int kw3  = (W - 2 * pad) / 3;
    auto plK3 = [&](juce::Slider& s, juce::Label& l, int col)
    {
        int x = pad + col * kw3 + (kw3 - Km) / 2;
        s.setBounds(x, kY2,          Km, Km);
        l.setBounds(pad + col * kw3, kY2 + Km + 3, kw3, 10);
    };
    plK3(sDrive,    lDrive,   0);
    plK3(sKeyTrack, lKey,     1);
    plK3(sVelTrack, lVel,     2);
}
