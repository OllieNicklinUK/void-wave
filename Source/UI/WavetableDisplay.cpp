#include "WavetableDisplay.h"
#include "LookAndFeel.h"

static const juce::Colour C1 { VW::SEC_OSC1  };   // OSC1 amber
static const juce::Colour C2 { VW::SEC_OSC2  };   // OSC2 orange

static const char* TABLE_NAMES[] = {
    "SINE","SAW","SQUARE","SPECTRAL","FORMANT A","FORMANT E",
    "BASS 01","BASS 02","NOISE"
};
static constexpr int N_TABLES = 9;

// ── Wave generation (varies meaningfully with pos 0→1) ────────────────────────

std::vector<float> WavetableDisplay::generateWave(int tableIndex, float pos, int n)
{
    std::vector<float> out(static_cast<size_t>(n));
    const float twoPi = juce::MathConstants<float>::twoPi;

    for (int i = 0; i < n; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(n);
        float a = t * twoPi;
        float s = 0.0f;

        switch (tableIndex % N_TABLES)
        {
            case 0: // SINE → adds harmonics as pos rises
                s = std::sin(a)
                  + pos * 0.5f * std::sin(2.0f * a)
                  + pos * 0.25f * std::sin(3.0f * a)
                  + pos * 0.1f  * std::sin(4.0f * a);
                break;

            case 1: // SAW → brightens, adds sub
                s = 2.0f * t - 1.0f;
                s += pos * 0.35f * std::sin(2.0f * a);
                s -= pos * 0.15f * std::sin(0.5f * a);
                break;

            case 2: // SQUARE → pulse-width modulates 0.5→0.15
            {
                float pw = 0.5f - pos * 0.35f;
                s = (t < pw) ? 1.0f : -1.0f;
                // soften edges slightly
                float edge = 0.05f;
                float dist = std::min(std::abs(t - pw), std::min(t, 1.0f - t));
                if (dist < edge) s *= dist / edge;
                break;
            }

            case 3: // SPECTRAL → complex harmonic sweep
                for (int h = 1; h <= 8; ++h)
                {
                    float phase = pos * juce::MathConstants<float>::pi * h;
                    s += (1.0f / h) * std::sin(h * a + phase);
                }
                s *= 0.5f;
                break;

            case 4: // FORMANT A → vowel formant morph
            case 5: // FORMANT E
            {
                float f1 = 700.0f  + pos * 1300.0f;
                float f2 = 1100.0f + pos * 1800.0f;
                float fund = 120.0f;
                for (int h = 1; h <= 16; ++h)
                {
                    float fh = h * fund;
                    float g = std::exp(-std::pow((fh - f1) / 250.0f, 2.0f))
                            + 0.6f * std::exp(-std::pow((fh - f2) / 350.0f, 2.0f));
                    s += g * std::sin(h * a);
                }
                s *= 0.12f;
                break;
            }

            case 6: // BASS 01 → sub with rumble that builds
            case 7: // BASS 02
                s = std::sin(a) * (1.0f - pos * 0.4f)
                  + std::sin(2.0f * a) * pos * 0.6f
                  + std::sin(0.5f * a) * pos * 0.3f;
                break;

            default: // NOISE → pseudo spectral noise
                s = std::sin(a + pos * std::sin(3.0f * a) * 5.0f)
                  * std::cos(pos * a * 2.0f + std::sin(7.0f * a) * pos);
                break;
        }
        out[static_cast<size_t>(i)] = juce::jlimit(-1.0f, 1.0f, s);
    }
    return out;
}

juce::String WavetableDisplay::tableName(int idx)
{
    return TABLE_NAMES[juce::jlimit(0, N_TABLES - 1, idx % N_TABLES)];
}

// ── Frame cache ───────────────────────────────────────────────────────────────

void WavetableDisplay::rebuildCache(int tableIdx, std::vector<std::vector<float>>& cache)
{
    cache.resize(static_cast<size_t>(N_FRAMES));
    for (int i = 0; i < N_FRAMES; ++i)
    {
        float pos = static_cast<float>(i) / static_cast<float>(N_FRAMES - 1);
        cache[static_cast<size_t>(i)] = generateWave(tableIdx, pos);
    }
}

// ── Constructor ───────────────────────────────────────────────────────────────

WavetableDisplay::WavetableDisplay(VoidWaveAudioProcessor& p) : processor(p)
{
    auto& t = p.apvts;

    sOscMix.setSliderStyle(juce::Slider::LinearHorizontal);
    sOscMix.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    sOscMix.setColour(juce::Slider::thumbColourId, juce::Colour(VW::TEXT_BRIGHT));
    sOscMix.setColour(juce::Slider::trackColourId, C1.withAlpha(0.4f));
    addAndMakeVisible(sOscMix);

    cMixMode.setColour(juce::ComboBox::backgroundColourId, juce::Colour(VW::BG_RAISED));
    cMixMode.setColour(juce::ComboBox::textColourId,       juce::Colour(VW::TEXT_BRIGHT));
    cMixMode.setColour(juce::ComboBox::outlineColourId,    juce::Colour(VW::BORDER_VIS));
    cMixMode.setColour(juce::ComboBox::arrowColourId,      C1);
    if (auto* mp = dynamic_cast<juce::AudioParameterChoice*>(t.getParameter("osc_mix_mode")))
        cMixMode.addItemList(mp->choices, 1);
    addAndMakeVisible(cMixMode);

    lMix.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.0f, juce::Font::plain));
    lMix.setColour(juce::Label::textColourId, juce::Colour(VW::TEXT_MID));
    addAndMakeVisible(lMix);

    attMix  = std::make_unique<SlAtt>(t, "osc_mix",      sOscMix);
    attMode = std::make_unique<CbAtt>(t, "osc_mix_mode", cMixMode);

    pOsc1Table = t.getRawParameterValue("osc1_table");
    pOsc1Pos   = t.getRawParameterValue("osc1_position");
    pOsc1Mode  = t.getRawParameterValue("osc1_wt_mode");
    pOsc2Table = t.getRawParameterValue("osc2_table");
    pOsc2Pos   = t.getRawParameterValue("osc2_position");
    pOsc2Mode  = t.getRawParameterValue("osc2_wt_mode");
    pOscMix    = t.getRawParameterValue("osc_mix");

    startTimerHz(30);
}

WavetableDisplay::~WavetableDisplay() { stopTimer(); }

// ── Timer ─────────────────────────────────────────────────────────────────────

void WavetableDisplay::timerCallback()
{
    animTick += 1.0f / 30.0f;   // seconds elapsed

    // Advance scan display independently for each osc
    if (pOsc1Mode && pOsc1Pos)
    {
        if (static_cast<int>(pOsc1Mode->load()) == 2)  // SCAN
        {
            float rate = pOsc1Pos->load() * 4.0f;  // 0–4 Hz (mirrors DSP)
            scanPos1 += rate / 30.0f;
            while (scanPos1 >= 1.0f) scanPos1 -= 1.0f;
        }
        else
        {
            scanPos1 = pOsc1Pos->load();
        }
    }

    if (pOsc2Mode && pOsc2Pos)
    {
        if (static_cast<int>(pOsc2Mode->load()) == 2)
        {
            float rate = pOsc2Pos->load() * 4.0f;
            scanPos2 += rate / 30.0f;
            while (scanPos2 >= 1.0f) scanPos2 -= 1.0f;
        }
        else
        {
            scanPos2 = pOsc2Pos->load();
        }
    }

    repaint();
}

// ── 3D Waterfall ─────────────────────────────────────────────────────────────

static void drawWaterfallOsc(juce::Graphics& g,
                              const std::vector<std::vector<float>>& frames,
                              juce::Colour col, juce::Rectangle<float> canvas,
                              float currentPos,   // 0–1
                              int   N,
                              float opacity = 1.0f)
{
    if (frames.empty()) return;

    // Geometry: frames go from front (bottom-left) to back (top-right)
    const float stepX  = 14.0f;
    const float stepY  = 5.0f;
    const float waveW  = canvas.getWidth()  - (N - 1) * stepX;
    const float waveAmp = (canvas.getHeight() - (N - 1) * stepY) * 0.36f;

    const int currentFrame = static_cast<int>(std::round(currentPos * (N - 1)));
    const int ns = static_cast<int>(frames[0].size());

    // Draw back → front so front sits on top
    for (int ki = N - 1; ki >= 0; --ki)
    {
        const auto& wave = frames[static_cast<size_t>(ki)];

        float bx  = canvas.getX()    + ki * stepX;
        float by  = canvas.getBottom() - ki * stepY;  // baseline y
        float cy  = by - waveAmp;                      // centre y

        float dist  = static_cast<float>(std::abs(ki - currentFrame));
        float bright = std::exp(-dist * 0.45f);
        bool  isCur  = (ki == currentFrame);

        // Fill under the waveform
        juce::Path fill;
        fill.startNewSubPath(bx, by);
        for (int si = 0; si < ns; ++si)
        {
            float px = bx + static_cast<float>(si) / static_cast<float>(ns - 1) * waveW;
            float py = cy - wave[static_cast<size_t>(si)] * waveAmp;
            si == 0 ? fill.lineTo(px, py) : fill.lineTo(px, py);
        }
        fill.lineTo(bx + waveW, by);
        fill.closeSubPath();

        juce::ColourGradient grad(
            col.withAlpha(bright * (isCur ? 0.28f : 0.10f) * opacity), bx, cy,
            col.withAlpha(0.0f),                                         bx, by, false);
        g.setGradientFill(grad);
        g.fillPath(fill);

        // Waveform line
        juce::Path line;
        for (int si = 0; si < ns; ++si)
        {
            float px = bx + static_cast<float>(si) / static_cast<float>(ns - 1) * waveW;
            float py = cy - wave[static_cast<size_t>(si)] * waveAmp;
            si == 0 ? line.startNewSubPath(px, py) : line.lineTo(px, py);
        }

        if (isCur)
        {
            g.setColour(col.withAlpha(0.25f * opacity));
            g.strokePath(line, juce::PathStrokeType(3.5f));
            g.setColour(col.withAlpha(0.95f * opacity));
            g.strokePath(line, juce::PathStrokeType(1.5f));
        }
        else
        {
            g.setColour(col.withAlpha(bright * 0.5f * opacity));
            g.strokePath(line, juce::PathStrokeType(0.8f));
        }
    }
}

// ── 3D perspective grid ───────────────────────────────────────────────────────

static void drawPerspectiveGrid(juce::Graphics& g, juce::Rectangle<float> canvas, int nFrames)
{
    // Matches the same step geometry as drawWaterfallOsc
    const float stepX = 14.0f;
    const float stepY = 5.0f;
    const float waveW = canvas.getWidth() - (nFrames - 1) * stepX;

    // Colour: knob-body grey, barely visible
    const juce::Colour grid(0xff3a3f55);

    // ── Frame baseline rows (left-right at each depth level) ─────────────────
    for (int k = 0; k < nFrames; ++k)
    {
        float bx = canvas.getX() + k * stepX;
        float by = canvas.getBottom() - k * stepY;
        // Back rows slightly brighter — creates natural aerial perspective
        float a = 0.07f + static_cast<float>(k) / (nFrames - 1) * 0.08f;
        g.setColour(grid.withAlpha(a));
        g.drawLine(bx, by, bx + waveW, by, 0.6f);
    }

    // ── Depth convergence columns (front to back, evenly spaced across width) ─
    const int nCols = 7;
    for (int di = 0; di <= nCols; ++di)
    {
        float t  = static_cast<float>(di) / static_cast<float>(nCols);
        float x0 = canvas.getX() + t * waveW;
        float y0 = canvas.getBottom();
        float x1 = x0 + (nFrames - 1) * stepX;
        float y1 = canvas.getBottom() - (nFrames - 1) * stepY;
        g.setColour(grid.withAlpha(0.09f));
        g.drawLine(x0, y0, x1, y1, 0.5f);
    }
}

// ── Paint ─────────────────────────────────────────────────────────────────────

void WavetableDisplay::paint(juce::Graphics& g)
{
    const int W = getWidth(), H = getHeight();
    const int pad = 8;

    g.fillAll(juce::Colour(VW::BG_PANEL));

    // Header
    g.setColour(juce::Colour(VW::SEC_WT));
    g.fillRect(0, 0, W, 14);
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xffffffff));
    g.drawText("WAVETABLE", 0, 0, W, 14, juce::Justification::centred);
    g.setColour(juce::Colour(VW::BORDER_SUB));
    g.drawRect(getLocalBounds(), 1);

    // Canvas
    auto canvas = juce::Rectangle<float>(
        pad, 18.0f,
        W - 2 * pad, H - 18.0f - 44.0f - 4.0f);

    g.setColour(juce::Colour(VW::BG_PANEL));
    g.fillRoundedRectangle(canvas, 4.0f);
    g.setColour(juce::Colour(VW::BORDER_VIS));
    g.drawRoundedRectangle(canvas, 4.0f, 1.0f);

    // Read params
    int   t1  = pOsc1Table ? static_cast<int>(pOsc1Table->load()) : 0;
    int   t2  = pOsc2Table ? static_cast<int>(pOsc2Table->load()) : 0;
    float mix = pOscMix    ? pOscMix->load() : 0.0f;

    // Rebuild caches if table changed
    if (t1 != cachedTable1) { rebuildCache(t1, frames1); cachedTable1 = t1; }
    if (t2 != cachedTable2) { rebuildCache(t2, frames2); cachedTable2 = t2; }

    // Perspective grid — drawn first so it sits behind everything
    drawPerspectiveGrid(g, canvas.reduced(2, 0), N_FRAMES);

    // Draw OSC2 — opacity tracks the mix slider directly
    if (mix > 0.01f && !frames2.empty())
    {
        auto canvas2 = canvas.reduced(0, 4).translated(0, 4);
        drawWaterfallOsc(g, frames2, C2,
                         canvas2.reduced(2, 0),
                         scanPos2, N_FRAMES, mix);
    }

    // Draw OSC1 stack
    if (!frames1.empty())
        drawWaterfallOsc(g, frames1, C1,
                         canvas.reduced(2, 0),
                         scanPos1, N_FRAMES);

    // Table name click zones + labels
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 9.0f, juce::Font::bold));

    // OSC1 — top-left of canvas
    int z1x = (int)canvas.getX() + 6;
    int z1y = (int)canvas.getY() + 4;
    osc1NameZone = { z1x - 2, z1y - 2, 90, 14 };
    g.setColour(C1.withAlpha(0.15f));
    g.fillRoundedRectangle(osc1NameZone.toFloat(), 3.0f);
    g.setColour(C1);
    g.drawText(tableName(t1), z1x, z1y, 86, 10, juce::Justification::centredLeft);

    // OSC2 — top-right of canvas (only if mixed in)
    int z2x = (int)canvas.getRight() - 96;
    int z2y = (int)canvas.getY() + 4;
    osc2NameZone = { z2x - 2, z2y - 2, 94, 14 };
    if (mix > 0.01f)
    {
        g.setColour(C2.withAlpha(0.15f * mix));
        g.fillRoundedRectangle(osc2NameZone.toFloat(), 3.0f);
        g.setColour(C2.withAlpha(0.85f * mix));
        g.drawText(tableName(t2), z2x, z2y, 90, 10, juce::Justification::centredRight);
    }

    // Position readout — bottom of canvas
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 7.0f, juce::Font::plain));
    bool scan1 = pOsc1Mode && static_cast<int>(pOsc1Mode->load()) == 2;
    juce::String posStr = scan1
        ? juce::String("SCAN  ") + juce::String(static_cast<int>(scanPos1 * 255)) + "/255"
        : juce::String("POS ") + juce::String(static_cast<int>(scanPos1 * 255)) + "/255";
    g.setColour(C1.withAlpha(0.45f));
    g.drawText(posStr, (int)canvas.getX() + 4, (int)canvas.getBottom() - 12,
               140, 10, juce::Justification::centredLeft);
}

void WavetableDisplay::resized()
{
    const int W = getWidth(), H = getHeight(), pad = 8;
    int bottomY = H - 40;
    lMix   .setBounds(pad,          bottomY,      30, 12);
    sOscMix.setBounds(pad + 32,     bottomY,      W - pad * 2 - 32 - pad - 120, 12);
    cMixMode.setBounds(W - pad - 118, bottomY,    118, 16);
}

// ── Mouse — click table name to change ───────────────────────────────────────

void WavetableDisplay::mouseUp(const juce::MouseEvent& e)
{
    if (osc1NameZone.contains(e.getPosition()))
        showTableMenu(true);
    else if (osc2NameZone.contains(e.getPosition()))
        showTableMenu(false);
}

void WavetableDisplay::showTableMenu(bool isOsc1)
{
    juce::PopupMenu menu;
    for (int i = 0; i < N_TABLES; ++i)
        menu.addItem(i + 1, TABLE_NAMES[i]);

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
        [this, isOsc1](int result)
        {
            if (result <= 0) return;
            juce::String paramID = isOsc1 ? "osc1_table" : "osc2_table";
            if (auto* param = processor.apvts.getParameter(paramID))
                param->setValueNotifyingHost(
                    param->convertTo0to1(static_cast<float>(result - 1)));
        });
}
