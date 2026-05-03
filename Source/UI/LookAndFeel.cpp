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

// ── Rotary knob — GR-8 style: flat body + needle indicator ───────────────────
// Reference: Phuturetone GR-8 (dark grey circular body, thin white pointer line)

void VoidWaveLookAndFeel::drawRotarySlider(juce::Graphics& g,
    int x, int y, int width, int height,
    float sliderPos, float startAngle, float endAngle,
    juce::Slider& slider)
{
    const float cx  = x + width  * 0.5f;
    const float cy  = y + height * 0.5f;
    const float r   = (juce::jmin(width, height) * 0.5f) - 2.0f;
    const float valueAngle = startAngle + sliderPos * (endAngle - startAngle);

    // ── Outer drop shadow ─────────────────────────────────────────────────────
    g.setColour(juce::Colour(0x55000000));
    g.fillEllipse(cx - r + 1.0f, cy - r + 2.5f, r * 2.0f, r * 2.0f);  // shadow offset

    // ── Knob body — dark metallic flat circle ─────────────────────────────────
    {
        // Base fill: dark grey (#222226)
        g.setColour(juce::Colour(0xff222226));
        g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);

        // Subtle top-left specular (single arc, very faint)
        juce::Path specular;
        specular.addCentredArc(cx, cy, r * 0.9f, r * 0.9f, 0.0f,
                               juce::MathConstants<float>::pi * 1.3f,
                               juce::MathConstants<float>::pi * 1.7f, true);
        g.setColour(juce::Colour(0x14ffffff));
        g.strokePath(specular, juce::PathStrokeType(r * 0.25f));

        // Bottom shadow inner edge
        juce::Path shadow;
        shadow.addCentredArc(cx, cy, r * 0.9f, r * 0.9f, 0.0f,
                              juce::MathConstants<float>::pi * 0.2f,
                              juce::MathConstants<float>::pi * 0.8f, true);
        g.setColour(juce::Colour(0x20000000));
        g.strokePath(shadow, juce::PathStrokeType(r * 0.22f));

        // Outer rim: slightly lighter ring
        g.setColour(juce::Colour(0xff2e2e34));
        g.drawEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f, 1.2f);
    }

    // ── Subtle track arc (very faint, just shows the sweep range) ────────────
    {
        juce::Path track;
        track.addCentredArc(cx, cy, r - 2.5f, r - 2.5f, 0.0f, startAngle, endAngle, true);
        g.setColour(juce::Colour(0x18ffffff));
        g.strokePath(track, juce::PathStrokeType(1.0f,
                         juce::PathStrokeType::curved, juce::PathStrokeType::butt));
    }

    // ── Value arc mark (accent colour, thin) ─────────────────────────────────
    if (sliderPos > 0.005f)
    {
        juce::Path varc;
        varc.addCentredArc(cx, cy, r - 2.5f, r - 2.5f, 0.0f, startAngle, valueAngle, true);
        g.setColour(accent.withAlpha(0.55f));
        g.strokePath(varc, juce::PathStrokeType(1.5f,
                         juce::PathStrokeType::curved, juce::PathStrokeType::butt));
    }

    // ── Needle indicator — thin pointer line from near-centre to near-rim ─────
    {
        float lineStart = r * 0.22f;   // start slightly off-centre
        float lineEnd   = r * 0.78f;   // end well inside rim
        float sx = cx + lineStart * std::sin(valueAngle);
        float sy = cy - lineStart * std::cos(valueAngle);
        float ex = cx + lineEnd   * std::sin(valueAngle);
        float ey = cy - lineEnd   * std::cos(valueAngle);

        // Shadow of needle
        g.setColour(juce::Colour(0x55000000));
        g.drawLine(sx + 0.6f, sy + 0.8f, ex + 0.6f, ey + 0.8f, 1.5f);

        // Needle — bright white, clean
        g.setColour(juce::Colour(0xffe8edf8));
        g.drawLine(sx, sy, ex, ey, 1.5f);

        // Small bright dot at needle tip
        g.setColour(accent.withAlpha(0.9f));
        g.fillEllipse(ex - 1.8f, ey - 1.8f, 3.6f, 3.6f);
    }

    // ── Value readout on hover ────────────────────────────────────────────────
    if (slider.isMouseOverOrDragging() && slider.getTextBoxPosition() == juce::Slider::NoTextBox)
    {
        g.setColour(juce::Colour(0xffe8edf8).withAlpha(0.85f));
        g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(),
                             juce::jmax(6.0f, r * 0.42f), juce::Font::plain));
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

    // Fill
    juce::Colour fill = juce::Colour(VW::BG_RAISED);
    if (down || on)        fill = accent.withAlpha(0.18f);
    else if (highlighted)  fill = juce::Colour(VW::BG_HIGH);
    g.setColour(fill);
    g.fillRoundedRectangle(b, 3.0f);

    // Border
    juce::Colour border = (on || down)
        ? accent.withAlpha(0.8f)
        : (highlighted ? juce::Colour(VW::BORDER_VIS) : juce::Colour(VW::BORDER_SUB));
    g.setColour(border);
    g.drawRoundedRectangle(b, 3.0f, 1.0f);

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
