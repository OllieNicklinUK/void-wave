#include "MidiLearnOverlay.h"
#include "LookAndFeel.h"

static const juce::Colour AMBER   { 0xfff5a623 };
static const juce::Colour GREEN   { 0xff4cff91 };
static const juce::Colour WHITE   { 0xffe8eaf0 };

MidiLearnOverlay::MidiLearnOverlay(VoidWaveAudioProcessor& p, juce::Component& root)
    : processor(p), rootComp(root)
{
    setInterceptsMouseClicks(false, false);  // transparent by default
}

MidiLearnOverlay::~MidiLearnOverlay()
{
    rootComp.removeMouseListener(this);
}

void MidiLearnOverlay::setActive(bool active)
{
    if (active)
    {
        setInterceptsMouseClicks(false, false);  // overlay doesn't block, listener does
        rootComp.addMouseListener(this, true);   // listen to all children
    }
    else
    {
        rootComp.removeMouseListener(this);
        processor.midiLearnTarget = {};
    }
    repaint();
}

// ── Mouse ──────────────────────────────────────────────────────────────────────

void MidiLearnOverlay::mouseDown(const juce::MouseEvent& e)
{
    if (!processor.midiLearnActive) return;

    auto* comp = e.eventComponent;
    if (!comp || comp == this) return;

    juce::String paramId = comp->getComponentID();
    if (paramId.isNotEmpty())
    {
        processor.midiLearnTarget = paramId;
        repaint();
    }
}

// ── Paint ──────────────────────────────────────────────────────────────────────

void MidiLearnOverlay::drawHighlights(juce::Graphics& g, juce::Component* node) const
{
    if (!node) return;

    for (auto* child : node->getChildren())
    {
        if (child == this || !child->isVisible()) continue;

        juce::String paramId = child->getComponentID();
        if (paramId.isNotEmpty())
        {
            auto bounds = getLocalArea(child, child->getLocalBounds()).toFloat().reduced(1.f);
            bool isTarget = (paramId == processor.midiLearnTarget);
            int  cc       = processor.ccForParam(paramId);
            bool isMapped = (cc >= 0);

            if (isTarget)
            {
                // Bright amber fill + white outline — waiting for CC
                g.setColour(AMBER.withAlpha(0.2f));
                g.fillRoundedRectangle(bounds, 4.f);
                g.setColour(WHITE.withAlpha(0.9f));
                g.drawRoundedRectangle(bounds, 4.f, 2.f);
                // CC label if already mapped
                if (isMapped)
                {
                    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 7.f, juce::Font::bold));
                    g.setColour(WHITE);
                    g.drawText("CC" + juce::String(cc), bounds, juce::Justification::centred, false);
                }
            }
            else if (isMapped)
            {
                // Green outline + CC label — already mapped
                g.setColour(GREEN.withAlpha(0.15f));
                g.fillRoundedRectangle(bounds, 4.f);
                g.setColour(GREEN.withAlpha(0.8f));
                g.drawRoundedRectangle(bounds, 4.f, 1.5f);
                g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 7.f, juce::Font::plain));
                g.setColour(GREEN.withAlpha(0.85f));
                g.drawText("CC" + juce::String(cc), bounds, juce::Justification::centred, false);
            }
            else
            {
                // Dim amber outline — mappable but not yet mapped
                g.setColour(AMBER.withAlpha(0.18f));
                g.drawRoundedRectangle(bounds, 4.f, 1.f);
            }
        }

        drawHighlights(g, child);
    }
}

void MidiLearnOverlay::paint(juce::Graphics& g)
{
    if (!processor.midiLearnActive) return;

    // Dim background tint
    g.setColour(juce::Colour(0xff0a0c10).withAlpha(0.35f));
    g.fillRect(getLocalBounds());

    drawHighlights(g, &rootComp);

    // Header instruction
    g.setColour(AMBER);
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 9.f, juce::Font::bold));
    g.drawText(processor.midiLearnTarget.isEmpty()
               ? "MIDI MAP: click a knob to select, then move a MIDI CC"
               : "MIDI MAP: move a MIDI CC to assign to  [" + processor.midiLearnTarget + "]",
               getLocalBounds().removeFromTop(56), juce::Justification::centred, false);
}
