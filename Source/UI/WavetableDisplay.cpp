#include "WavetableDisplay.h"
#include "LookAndFeel.h"

static const juce::Colour AMBER  { VW::NEON_AMBER };
static const juce::Colour PURPLE { VW::NEON_VIOLET };

// ── Wave generation ────────────────────────────────────────────────────────────

juce::String WavetableDisplay::tableName(int idx)
{
    static const char* names[] = { "SINE","SAW","SQUARE","SPECTRAL","BASS","FORMANT","NOISE","USER" };
    return names[juce::jlimit(0, 7, idx % 8)];
}

std::vector<float> WavetableDisplay::generateWave(int tableIndex, float positionBlend, int numSamples)
{
    std::vector<float> out(static_cast<size_t>(numSamples));
    int shape = tableIndex % 3;  // 0=sine 1=saw 2=square (maps to WavetableBank tables)

    for (int i = 0; i < numSamples; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(numSamples);
        float angle = t * juce::MathConstants<float>::twoPi;
        float s = 0.0f;

        switch (shape)
        {
            case 0: s = std::sin(angle); break;
            case 1: s = 2.0f * t - 1.0f; break;  // saw
            case 2: s = t < 0.5f ? 1.0f : -1.0f; break;  // square
            default: s = std::sin(angle); break;
        }

        // Blend with position: morph subtly toward triangle at high position
        float tri = std::asin(std::sin(angle)) / (juce::MathConstants<float>::halfPi);
        out[static_cast<size_t>(i)] = s * (1.0f - positionBlend * 0.35f)
                                    + tri * positionBlend * 0.35f;
    }
    return out;
}

// ── Glow waveform drawing ─────────────────────────────────────────────────────

void WavetableDisplay::drawGlowWave(juce::Graphics& g, const std::vector<float>& data,
                                     juce::Colour col, juce::Rectangle<float> area,
                                     float phaseOffset, float alpha) const
{
    if (data.empty()) return;
    const int n  = static_cast<int>(data.size());
    float x0 = area.getX(), w = area.getWidth();
    float cy = area.getCentreY(), amp = area.getHeight() * 0.42f;

    // Phase shift: scroll by phaseOffset samples
    auto sample = [&](int i) -> float {
        int idx = (i + static_cast<int>(phaseOffset * n)) % n;
        if (idx < 0) idx += n;
        return data[static_cast<size_t>(idx)];
    };

    // Three passes: wide blur → medium → thin crisp
    struct Pass { float width; float a; };
    Pass passes[] = { {5.0f, 0.06f}, {2.5f, 0.18f}, {1.5f, 0.85f} };

    for (const auto& pass : passes)
    {
        juce::Path path;
        path.startNewSubPath(x0, cy - sample(0) * amp);
        for (int i = 1; i < n; ++i)
            path.lineTo(x0 + static_cast<float>(i) / static_cast<float>(n - 1) * w,
                        cy - sample(i) * amp);

        g.setColour(col.withAlpha(alpha * pass.a));
        g.strokePath(path, juce::PathStrokeType(pass.width));
    }
}

// ── Constructor ───────────────────────────────────────────────────────────────

WavetableDisplay::WavetableDisplay(VoidWaveAudioProcessor& p) : processor(p)
{
    auto& t = p.apvts;

    sOscMix.setSliderStyle(juce::Slider::LinearHorizontal);
    sOscMix.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    sOscMix.setColour(juce::Slider::thumbColourId,  juce::Colour(VW::TEXT_BRIGHT));
    sOscMix.setColour(juce::Slider::trackColourId,  AMBER.withAlpha(0.4f));
    addAndMakeVisible(sOscMix);

    cMixMode.setColour(juce::ComboBox::backgroundColourId, juce::Colour(VW::BG_RAISED));
    cMixMode.setColour(juce::ComboBox::textColourId,       juce::Colour(VW::TEXT_BRIGHT));
    cMixMode.setColour(juce::ComboBox::outlineColourId,    juce::Colour(VW::BORDER_VIS));
    cMixMode.setColour(juce::ComboBox::arrowColourId,      AMBER);
    if (auto* mp = dynamic_cast<juce::AudioParameterChoice*>(t.getParameter("osc_mix_mode")))
        cMixMode.addItemList(mp->choices, 1);
    addAndMakeVisible(cMixMode);

    for (auto* l : { &lOsc1, &lOsc2, &lMix })
    {
        l->setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.0f, juce::Font::plain));
        l->setColour(juce::Label::textColourId, juce::Colour(VW::TEXT_MID));
        addAndMakeVisible(l);
    }
    lOsc1.setColour(juce::Label::textColourId, AMBER);
    lOsc2.setColour(juce::Label::textColourId, AMBER.withAlpha(0.6f));

    attMix  = std::make_unique<SlAtt>(t, "osc_mix",      sOscMix);
    attMode = std::make_unique<CbAtt>(t, "osc_mix_mode", cMixMode);

    // Cache param pointers for paint()
    pOsc1Table  = t.getRawParameterValue("osc1_table");
    pOsc1Pos    = t.getRawParameterValue("osc1_position");
    pOsc2Table  = t.getRawParameterValue("osc2_table");
    pOsc2Pos    = t.getRawParameterValue("osc2_position");
    pOscMix     = t.getRawParameterValue("osc_mix");

    startTimerHz(30);
}

WavetableDisplay::~WavetableDisplay() { stopTimer(); }

// ── Paint ─────────────────────────────────────────────────────────────────────

void WavetableDisplay::paint(juce::Graphics& g)
{
    const int W = getWidth(), H = getHeight();
    const int pad = 8;

    g.fillAll(juce::Colour(VW::BG_PANEL));

    // Section header
    g.setColour(AMBER);
    g.fillRect(0, 0, W, 14);
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xffffffff));
    g.drawText("WAVETABLE MONITOR", 0, 0, W, 14, juce::Justification::centred);
    g.setColour(juce::Colour(VW::BORDER_SUB));
    g.drawRect(getLocalBounds(), 1);

    // Canvas area (leaves room for controls at bottom)
    auto canvas = getLocalBounds()
                    .withTrimmedTop(18)
                    .withTrimmedBottom(46)
                    .reduced(pad, 2)
                    .toFloat();

    // Canvas background
    g.setColour(juce::Colour(VW::BG_PANEL));
    g.fillRoundedRectangle(canvas, 4.0f);
    g.setColour(juce::Colour(VW::BORDER_VIS));
    g.drawRoundedRectangle(canvas, 4.0f, 1.0f);

    // Subtle amber grid
    g.setColour(AMBER.withAlpha(0.04f));
    for (int i = 1; i < 4; ++i)
        g.drawHorizontalLine(static_cast<int>(canvas.getY() + canvas.getHeight() * i / 4.0f),
                             canvas.getX() + 4, canvas.getRight() - 4);
    for (int i = 1; i < 8; ++i)
        g.drawVerticalLine(static_cast<int>(canvas.getX() + canvas.getWidth() * i / 8.0f),
                           canvas.getY() + 4, canvas.getBottom() - 4);

    // Read current params
    int   osc1Tbl  = pOsc1Table ? static_cast<int>(pOsc1Table->load()) : 0;
    float osc1Pos  = pOsc1Pos   ? pOsc1Pos->load()  : 0.5f;
    int   osc2Tbl  = pOsc2Table ? static_cast<int>(pOsc2Table->load()) : 0;
    float osc2Pos  = pOsc2Pos   ? pOsc2Pos->load()  : 0.5f;
    float oscMix   = pOscMix    ? pOscMix->load()   : 0.5f;

    // Generate waveforms
    auto wave1 = generateWave(osc1Tbl, osc1Pos);
    auto wave2 = generateWave(osc2Tbl, osc2Pos);

    // OSC2 — dimmer, slight phase offset, visible if mix > 0
    if (oscMix > 0.02f)
        drawGlowWave(g, wave2, PURPLE, canvas,
                     animPhase * 0.8f + 0.25f,
                     juce::jlimit(0.1f, 0.5f, oscMix * 0.8f));

    // OSC1 — full brightness, scrolling
    drawGlowWave(g, wave1, AMBER, canvas,
                 animPhase,
                 juce::jlimit(0.2f, 1.0f, (1.0f - oscMix) * 0.8f + 0.3f));

    // Position indicator — dashed vertical line
    float posX = canvas.getX() + osc1Pos * canvas.getWidth();
    g.setColour(AMBER.withAlpha(0.55f));
    {
        juce::Path dashed;
        float y = canvas.getY() + 4;
        while (y < canvas.getBottom() - 4)
        {
            dashed.addLineSegment({ posX, y, posX, juce::jmin(y + 4.0f, canvas.getBottom() - 4) }, 1.0f);
            y += 7.0f;
        }
        g.strokePath(dashed, juce::PathStrokeType(1.5f));
    }

    // Position bar at bottom of canvas
    g.setColour(AMBER.withAlpha(0.12f));
    g.fillRect(canvas.getX(), canvas.getBottom() - 2.0f, canvas.getWidth(), 2.0f);
    {
        juce::ColourGradient grad { AMBER, canvas.getX(), canvas.getBottom(),
                                    AMBER.withAlpha(0.3f), canvas.getRight(), canvas.getBottom(), false };
        g.setGradientFill(grad);
        g.fillRect(canvas.getX(), canvas.getBottom() - 2.0f,
                   canvas.getWidth() * osc1Pos, 2.0f);
    }

    // Table name labels
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.0f, juce::Font::plain));
    g.setColour(AMBER.withAlpha(0.7f));
    g.drawText(tableName(osc1Tbl), (int)canvas.getX() + 5, (int)canvas.getY() + 4,
               80, 10, juce::Justification::centredLeft);
    g.setColour(PURPLE.withAlpha(0.5f));
    g.drawText(tableName(osc2Tbl), (int)canvas.getRight() - 85, (int)canvas.getY() + 4,
               80, 10, juce::Justification::centredRight);
}

void WavetableDisplay::resized()
{
    const int W = getWidth(), H = getHeight(), pad = 8;
    int bottomY = H - 40;
    lOsc1  .setBounds(pad,          bottomY,      40, 12);
    sOscMix.setBounds(pad + 42,     bottomY,      W - pad*2 - 42 - pad - 60, 12);
    lOsc2  .setBounds(W - pad - 60, bottomY,      40, 12);
    lMix   .setBounds(pad,          bottomY + 14, 40, 12);
    cMixMode.setBounds(pad + 42,    bottomY + 12, 120, 16);
}
