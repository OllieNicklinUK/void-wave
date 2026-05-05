#include "LookAndFeel.h"

// ── Construction ──────────────────────────────────────────────────────────────

VoidWaveLookAndFeel::VoidWaveLookAndFeel()
{
    // Base palette applied globally
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(VW::BG_DEEP));

    setColour(juce::Slider::thumbColourId,              juce::Colour(VW::NEON_AMBER));
    setColour(juce::Slider::trackColourId,              juce::Colour(VW::BG_CONTROL));
    setColour(juce::Slider::backgroundColourId,         juce::Colour(VW::BG_CONTROL));

    setColour(juce::Label::textColourId,                juce::Colour(VW::TEXT_MID));

    setColour(juce::ComboBox::backgroundColourId,       juce::Colour(VW::BG_RAISED));
    setColour(juce::ComboBox::textColourId,             juce::Colour(VW::TEXT_BRIGHT));
    setColour(juce::ComboBox::outlineColourId,          juce::Colour(VW::BORDER_VIS));
    setColour(juce::ComboBox::arrowColourId,            juce::Colour(VW::NEON_AMBER));
    setColour(juce::ComboBox::focusedOutlineColourId,   juce::Colour(VW::NEON_AMBER));

    setColour(juce::TextButton::buttonColourId,         juce::Colour(VW::BG_RAISED));
    setColour(juce::TextButton::buttonOnColourId,       juce::Colour(VW::NEON_AMBER).withAlpha(0.15f));
    setColour(juce::TextButton::textColourOffId,        juce::Colour(VW::TEXT_MID));
    setColour(juce::TextButton::textColourOnId,         juce::Colour(VW::NEON_AMBER));

    setColour(juce::PopupMenu::backgroundColourId,              juce::Colour(VW::BG_PANEL));
    setColour(juce::PopupMenu::textColourId,                    juce::Colour(VW::TEXT_BRIGHT));
    setColour(juce::PopupMenu::highlightedBackgroundColourId,   juce::Colour(VW::BG_HIGH));
    setColour(juce::PopupMenu::highlightedTextColourId,         juce::Colour(VW::NEON_AMBER));
    setColour(juce::PopupMenu::headerTextColourId,              juce::Colour(VW::NEON_AMBER).withAlpha(0.7f));

    setColour(juce::ToggleButton::tickColourId,         juce::Colour(VW::NEON_AMBER));
    setColour(juce::ToggleButton::tickDisabledColourId, juce::Colour(VW::TEXT_DIM));
}

// ── Shared helper: multi-pass neon arc ───────────────────────────────────────

void VoidWaveLookAndFeel::paintNeonArc(juce::Graphics& g,
                                        float cx, float cy, float radius,
                                        float startAngle, float endAngle,
                                        juce::Colour col, float strokeW, float glowAlpha)
{
    if (startAngle >= endAngle) return;

    // Pass 1 — wide soft glow
    juce::Path gp;
    gp.addCentredArc(cx, cy, radius, radius, 0.0f, startAngle, endAngle, true);
    g.setColour(col.withAlpha(glowAlpha * 0.5f));
    g.strokePath(gp, juce::PathStrokeType(strokeW + 5.0f,
                     juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Pass 2 — medium halo
    g.setColour(col.withAlpha(glowAlpha));
    g.strokePath(gp, juce::PathStrokeType(strokeW + 2.0f,
                     juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Pass 3 — crisp bright line
    g.setColour(col.withAlpha(0.95f));
    g.strokePath(gp, juce::PathStrokeType(strokeW,
                     juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

// ── Rotary knob — flat metallic disc + outer glow arc ────────────────────────
// Flat-top look: linear top→bottom gradient (overhead light on flat surface),
// not a sphere. Arc placed at halfW-4.5 so glow stays inside component bounds.

void VoidWaveLookAndFeel::drawRotarySlider(juce::Graphics& g,
    int x, int y, int width, int height,
    float sliderPos, float startAngle, float endAngle,
    juce::Slider& slider)
{
    const float cx   = x + width  * 0.5f;
    const float cy   = y + height * 0.5f;
    const float half = juce::jmin(width, height) * 0.5f;
    const float r    = half - 7.5f;   // body: 7.5 px inside component edge
    const float ar   = half - 4.5f;   // arc:  max glow = ar+2.25 = half-2.25 → no clip
    const float ang  = startAngle + sliderPos * (endAngle - startAngle);

    if (r < 4.0f) return;

    // ── 1. Drop shadow (contained within component) ───────────────────────────
    {
        juce::ColourGradient sh(
            juce::Colour(0x55000000), cx, cy + r * 0.35f,
            juce::Colour(0x00000000), cx, cy + r * 1.5f, true);
        g.setGradientFill(sh);
        g.fillEllipse(cx - r * 1.05f, cy - r * 0.65f + 2.5f,
                      r * 2.1f, r * 2.1f);
    }

    // ── 2. Knob body — LINEAR top→bottom gradient = flat metallic disc ────────
    // Key: NO radial centre highlight (that creates the sphere illusion).
    // Overhead studio light → slightly lighter at top, darker at bottom.
    {
        juce::ColourGradient disc(
            juce::Colour(0xff2e2e38), cx, cy - r,    // top: lighter
            juce::Colour(0xff111116), cx, cy + r,    // bottom: darker
            false);                                   // LINEAR, not radial
        disc.addColour(0.38, juce::Colour(0xff202028));
        disc.addColour(0.68, juce::Colour(0xff161620));
        g.setGradientFill(disc);
        g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);
    }

    // ── 3. Machined outer rim ─────────────────────────────────────────────────
    {
        // Hard dark outer ring (the cut edge)
        g.setColour(juce::Colour(0xff040407));
        g.drawEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f, 1.8f);

    }


    // ── 6. Value arc — three-pass glow, all strokes within component bounds ───
    if (sliderPos > 0.005f)
    {
        juce::Path varc;
        varc.addCentredArc(cx, cy, ar, ar, 0.0f, startAngle, ang, true);
        const auto curved = juce::PathStrokeType::curved;
        const auto rounded = juce::PathStrokeType::rounded;

        // Wide soft glow  (ar + 2.25 = half - 2.25 → no clip)
        g.setColour(accent.withAlpha(0.18f));
        g.strokePath(varc, juce::PathStrokeType(4.5f, curved, rounded));
        // Medium halo
        g.setColour(accent.withAlpha(0.42f));
        g.strokePath(varc, juce::PathStrokeType(2.4f, curved, rounded));
        // Crisp bright line
        g.setColour(accent.withAlpha(0.96f));
        g.strokePath(varc, juce::PathStrokeType(1.6f, curved, rounded));
    }

    // ── 7. Indicator — short bright notch at current angle ───────────────────
    {
        const float inner = r * 0.52f;
        const float outer = r * 0.88f;
        float sx = cx + inner * std::sin(ang);
        float sy = cy - inner * std::cos(ang);
        float ex = cx + outer * std::sin(ang);
        float ey = cy - outer * std::cos(ang);

        // Shadow
        g.setColour(juce::Colour(0x50000000));
        g.drawLine(sx + 0.5f, sy + 0.7f, ex + 0.5f, ey + 0.7f,
                   juce::jmax(1.0f, r * 0.095f));

        // Notch line — white with slight amber tint
        g.setColour(juce::Colour(0xfff0f0f8));
        g.drawLine(sx, sy, ex, ey, juce::jmax(1.0f, r * 0.095f));

        // Amber tip dot
        const float dr = juce::jmax(1.4f, r * 0.10f);
        g.setColour(accent.withAlpha(0.88f));
        g.fillEllipse(ex - dr, ey - dr, dr * 2.0f, dr * 2.0f);
    }

    // ── 8. Value readout on hover ─────────────────────────────────────────────
    if (slider.isMouseOverOrDragging() &&
        slider.getTextBoxPosition() == juce::Slider::NoTextBox)
    {
        g.setColour(juce::Colour(0xffe8edf8).withAlpha(0.92f));
        g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(),
                             juce::jmax(6.0f, r * 0.40f), juce::Font::plain));
        g.drawText(slider.getTextFromValue(slider.getValue()),
                   x, y, width, height, juce::Justification::centred, false);
    }
}

// ── Linear slider ─────────────────────────────────────────────────────────────

void VoidWaveLookAndFeel::drawLinearSlider(juce::Graphics& g,
    int x, int y, int width, int height,
    float sliderPos, float /*minPos*/, float /*maxPos*/,
    juce::Slider::SliderStyle style, juce::Slider& slider)
{
    const bool isHoriz  = (style == juce::Slider::LinearHorizontal);
    const bool isVert   = (style == juce::Slider::LinearVertical);
    const float trackW  = isVert ? 5.0f : 4.0f;
    const float r = 2.5f;

    if (isHoriz)
    {
        float trackY = y + height * 0.5f - trackW * 0.5f;

        // Track background
        g.setColour(juce::Colour(VW::BG_CONTROL));
        g.fillRoundedRectangle(static_cast<float>(x), trackY,
                               static_cast<float>(width), trackW, r);
        g.setColour(juce::Colour(VW::BORDER_SUB));
        g.drawRoundedRectangle(static_cast<float>(x), trackY,
                               static_cast<float>(width), trackW, r, 1.0f);

        // Value fill — neon with glow
        float fillW = sliderPos - static_cast<float>(x);
        if (fillW > 1.0f)
        {
            // Glow layer
            g.setColour(accent.withAlpha(0.25f));
            g.fillRoundedRectangle(static_cast<float>(x), trackY - 2.0f,
                                   fillW, trackW + 4.0f, r + 1.0f);
            // Solid fill
            g.setColour(accent.withAlpha(0.9f));
            g.fillRoundedRectangle(static_cast<float>(x), trackY, fillW, trackW, r);
        }
    }
    else if (isVert)
    {
        float trackX = x + width * 0.5f - trackW * 0.5f;
        float fullH  = static_cast<float>(height);
        float fillH  = static_cast<float>(y) + fullH - sliderPos;

        g.setColour(juce::Colour(VW::BG_CONTROL));
        g.fillRoundedRectangle(trackX, static_cast<float>(y),
                               trackW, fullH, r);
        g.setColour(juce::Colour(VW::BORDER_SUB));
        g.drawRoundedRectangle(trackX, static_cast<float>(y),
                               trackW, fullH, r, 1.0f);

        if (fillH > 1.0f)
        {
            g.setColour(accent.withAlpha(0.25f));
            g.fillRoundedRectangle(trackX - 2.0f, sliderPos,
                                   trackW + 4.0f, fillH, r + 1.0f);
            g.setColour(accent.withAlpha(0.9f));
            g.fillRoundedRectangle(trackX, sliderPos, trackW, fillH, r);
        }
    }

    juce::ignoreUnused(slider);
}

// ── Button background ─────────────────────────────────────────────────────────

void VoidWaveLookAndFeel::drawButtonBackground(juce::Graphics& g,
    juce::Button& button, const juce::Colour& /*bg*/,
    bool highlighted, bool down)
{
    auto b  = button.getLocalBounds().toFloat().reduced(0.5f);
    bool on = button.getToggleState();

    // Fill — respect transparent buttonColourId (lets PNG show through)
    juce::Colour base = button.findColour(juce::TextButton::buttonColourId);
    juce::Colour fill = base;
    if (down || on)       fill = accent.withAlpha(0.18f);
    else if (highlighted) fill = base.isTransparent() ? juce::Colour(VW::BG_HIGH).withAlpha(0.5f)
                                                        : juce::Colour(VW::BG_HIGH);

    if (fill.getAlpha() > 0)
    {
        g.setColour(fill);
        g.fillRoundedRectangle(b, 3.0f);
    }

    // Border — skip border entirely for transparent buttons
    if (!base.isTransparent())
    {
        juce::Colour border = (on || down)
            ? accent.withAlpha(0.8f)
            : (highlighted ? juce::Colour(VW::BORDER_VIS) : juce::Colour(VW::BORDER_SUB));
        g.setColour(border);
        g.drawRoundedRectangle(b, 3.0f, 1.0f);
    }

    // Top neon line when active
    if (on || down)
    {
        g.setColour(accent);
        g.fillRect(b.getX() + 3.0f, b.getY(), b.getWidth() - 6.0f, 1.5f);
    }
}

// ── Combo box ─────────────────────────────────────────────────────────────────

void VoidWaveLookAndFeel::drawComboBox(juce::Graphics& g,
    int width, int height, bool /*isDown*/,
    int /*bx*/, int /*by*/, int /*bw*/, int /*bh*/, juce::ComboBox& box)
{
    auto b = juce::Rectangle<float>(0.0f, 0.0f,
                                     static_cast<float>(width),
                                     static_cast<float>(height));

    // Background
    g.setColour(juce::Colour(VW::BG_RAISED));
    g.fillRoundedRectangle(b, 3.0f);

    // Border — slightly brighter if focused
    bool focused = box.hasKeyboardFocus(true);
    g.setColour(focused ? accent.withAlpha(0.7f) : juce::Colour(VW::BORDER_VIS));
    g.drawRoundedRectangle(b.reduced(0.5f), 3.0f, 1.0f);

    // Arrow
    float arrowX = width - 14.0f;
    float arrowY = height * 0.5f;
    juce::Path arrow;
    arrow.addTriangle(arrowX - 4.0f, arrowY - 2.5f,
                      arrowX + 4.0f, arrowY - 2.5f,
                      arrowX,        arrowY + 2.5f);
    g.setColour(accent.withAlpha(0.8f));
    g.fillPath(arrow);
}

// ── Label ──────────────────────────────────────────────────────────────────────

void VoidWaveLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    g.setColour(label.findColour(juce::Label::textColourId));
    g.setFont(label.getFont());
    g.drawText(label.getText(), label.getLocalBounds(),
               label.getJustificationType(), true);
}
