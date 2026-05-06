#pragma once
#include <JuceHeader.h>

// VOID WAVE design system — Retro-Futurism + Dark OLED
// Generated from UI/UX Pro Max skill. See DESIGN.md for full specification.
namespace VW
{
    // ── Background tiers ─────────────────────────────────────────────────────
    inline constexpr uint32_t BG_VOID    = 0xff000000;   // OLED true black
    inline constexpr uint32_t BG_DEEP    = 0xff050508;   // base background
    inline constexpr uint32_t BG_PANEL   = 0xff0a0b10;   // section surface
    inline constexpr uint32_t BG_RAISED  = 0xff111318;   // elevated control bg
    inline constexpr uint32_t BG_CONTROL = 0xff1a1d26;   // knob / slider track
    inline constexpr uint32_t BG_HIGH    = 0xff1e2235;   // active / hovered

    // ── Borders ───────────────────────────────────────────────────────────────
    inline constexpr uint32_t BORDER_SUB = 0xff1c2030;   // hairline
    inline constexpr uint32_t BORDER_VIS = 0xff2b2a2f;   // visible border

    // ── Neon palette ──────────────────────────────────────────────────────────
    inline constexpr uint32_t NEON_AMBER  = 0xfff5a623;  // primary brand
    inline constexpr uint32_t NEON_CYAN   = 0xff00d4ff;  // OSC / WT
    inline constexpr uint32_t NEON_VIOLET = 0xff9b5fe0;  // LFO / mod
    inline constexpr uint32_t NEON_GREEN  = 0xff00ff88;  // envelope
    inline constexpr uint32_t NEON_RED    = 0xffff3355;  // danger / drive

    // ── Section accent colours — warm amber family, distinct roles ─────────
    // All share the amber DNA; brightness and warmth signal the section character.
    inline constexpr uint32_t SEC_OSC1   = 0xfff5a623;  // amber   — OSC 1 (brand)
    inline constexpr uint32_t SEC_OSC2   = 0xffff7820;  // orange  — OSC 2 (hotter)
    inline constexpr uint32_t SEC_FILTER = 0xffcc3800;  // red-orange — aggressive cut
    inline constexpr uint32_t SEC_ENV    = 0xffd4a800;  // gold    — soft, sustaining
    inline constexpr uint32_t SEC_LFO    = 0xffa07030;  // copper  — slow, organic
    inline constexpr uint32_t SEC_FX     = 0xff784020;  // bronze  — deep, subtle
    inline constexpr uint32_t SEC_WT     = 0xfff5a623;  // amber   — wavetable display

    // ── Text ──────────────────────────────────────────────────────────────────
    inline constexpr uint32_t TEXT_BRIGHT = 0xffe8edf8;
    inline constexpr uint32_t TEXT_MID    = 0xff7a8099;
    inline constexpr uint32_t TEXT_DIM    = 0xff3a3f55;

    // ── Helper: neon colour → its glow version (35% alpha) ───────────────────
    inline juce::Colour glow(uint32_t neon, float alpha = 0.35f)
    {
        return juce::Colour(neon).withAlpha(alpha);
    }
}

class VoidWaveLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VoidWaveLookAndFeel();

    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPos, float startAngle, float endAngle,
                          juce::Slider&) override;

    void drawLinearSlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPos, float minPos, float maxPos,
                          juce::Slider::SliderStyle, juce::Slider&) override;

    void drawButtonBackground(juce::Graphics&, juce::Button&,
                              const juce::Colour& bg,
                              bool highlighted, bool down) override;

    void drawComboBox(juce::Graphics&, int width, int height, bool isDown,
                      int bx, int by, int bw, int bh, juce::ComboBox&) override;

    void drawLabel(juce::Graphics&, juce::Label&) override;

    void setAccentColour(juce::Colour c) { accent = c; }
    juce::Colour getAccentColour()  const { return accent; }

private:
    juce::Colour accent { juce::Colour(VW::NEON_AMBER) };

    // Shared helpers
    static void paintNeonArc(juce::Graphics& g, float cx, float cy, float radius,
                              float startAngle, float endAngle,
                              juce::Colour col, float strokeW, float glowAlpha);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoidWaveLookAndFeel)
};
