#include "FilterSection.h"
#include "LookAndFeel.h"
static const juce::Colour AMBER { VW::SEC_FILTER };

// ── Frequency response drawing ────────────────────────────────────────────────

void FilterSection::drawFrequencyResponse(juce::Graphics& g, juce::Rectangle<float> area)
{
    if (!pFilterType || !pFilterCutoff || !pFilterRes) return;
    int type = static_cast<int>(pFilterType->load());
    float cutoff = pFilterCutoff->load();
    float res = pFilterRes->load();

    float w = area.getWidth(), h = area.getHeight(), x0 = area.getX(), y0 = area.getY();
    float bottom = y0 + h;

    // Map 20Hz-20kHz to 0..1 log scale
    auto freqToX = [](float f) {
        return (std::log10(std::max(20.0f, f)) - std::log10(20.0f)) / (std::log10(20000.0f) - std::log10(20.0f));
    };

    float normCutoff = freqToX(cutoff);
    float cx = x0 + normCutoff * w;
    float peakY = bottom - h * 0.75f; // Flat top of passband

    juce::Path p;
    const int pts = 64;
    for (int i = 0; i <= pts; ++i)
    {
        float xNorm = static_cast<float>(i) / pts;
        float px = x0 + xNorm * w;
        float y = bottom;
        float dist = xNorm - normCutoff;
        
        switch (type) {
            case 0: // LP12
            case 1: // LP24
                if (xNorm < normCutoff) y = peakY;
                else y = peakY + dist * h * (type == 1 ? 8.0f : 4.0f);
                break;
            case 2: // HP12
            case 3: // HP24
                if (xNorm > normCutoff) y = peakY;
                else y = peakY - dist * h * (type == 3 ? 8.0f : 4.0f);
                break;
            case 4: // BP
                y = peakY + std::abs(dist) * h * (4.0f + res * 6.0f);
                break;
            case 5: // Notch
                y = peakY;
                if (std::abs(dist) < 0.05f) y = bottom - std::abs(dist) * 20.0f * h;
                break;
            default: // Other
                y = peakY + std::sin(xNorm * 30.0f) * h * 0.1f * res;
                break;
        }
        
        // Add resonant peak
        if (type < 4) {
            float peakW = 0.04f * (1.0f - res * 0.5f);
            float peakInfluence = std::max(0.0f, 1.0f - std::abs(dist) / peakW);
            y -= peakInfluence * h * res * 0.8f;
        }
        else if (type == 4) { // BP peak
            y -= h * res * 0.5f;
        }

        y = juce::jlimit(y0, bottom, y);
        if (i == 0) p.startNewSubPath(px, y);
        else p.lineTo(px, y);
    }

    g.setColour(AMBER.withAlpha(0.15f));
    juce::Path fill = p;
    fill.lineTo(x0 + w, bottom);
    fill.lineTo(x0, bottom);
    fill.closeSubPath();
    g.fillPath(fill);

    g.setColour(AMBER.withAlpha(0.85f));
    g.strokePath(p, juce::PathStrokeType(1.5f));

    // Draw cutoff vertical line
    g.setColour(AMBER.withAlpha(0.4f));
    g.drawVerticalLine(static_cast<int>(cx), y0, bottom);
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

    // Filter pointers for live viz
    pFilterType   = t.getRawParameterValue("filter_type");
    pFilterCutoff = t.getRawParameterValue("filter_cutoff");
    pFilterRes    = t.getRawParameterValue("filter_res");

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

    drawFrequencyResponse(g, vizArea.reduced(2, 2));

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
