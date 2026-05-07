#include "PresetBrowser.h"
#include "LookAndFeel.h"

static const juce::Colour AMBER  { VW::NEON_AMBER };
static const juce::Colour BG     { VW::BG_DEEP };
static const juce::Colour BG_COL { VW::BG_PANEL };
static const juce::Colour BORDER { VW::BORDER_VIS };
static const juce::Colour TXT    { VW::TEXT_MID };

// ─────────────────────────────────────────────────────────────────────────────

PresetBrowser::PresetBrowser(VoidWaveAudioProcessor& p) : processor(p)
{
    setInterceptsMouseClicks(true, true);
    addKeyListener(this);
    setWantsKeyboardFocus(true);
    
    addAndMakeVisible(newPresetBtn);
    newPresetBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(VW::BG_RAISED));
    newPresetBtn.setColour(juce::TextButton::textColourOffId, AMBER);
    newPresetBtn.onClick = [this] {
        processor.presetManager.loadInitPreset();
        if (onClose) onClose();
    };

    addAndMakeVisible(exportAllBtn);
    exportAllBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(VW::BG_RAISED));
    exportAllBtn.setColour(juce::TextButton::textColourOffId, AMBER);
    exportAllBtn.onClick = [this] {
        fileChooser = std::make_unique<juce::FileChooser>("Export User Presets",
                                                          juce::File::getSpecialLocation(juce::File::userDesktopDirectory),
                                                          "");
        auto folderFlags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectDirectories;
        fileChooser->launchAsync(folderFlags, [this](const juce::FileChooser& fc)
        {
            juce::File targetDir = fc.getResult();
            if (targetDir.isDirectory())
            {
                int count = 0;
                auto& pm = processor.presetManager;
                for (int i = 0; i < pm.getNumPresets(); ++i) {
                    const auto& p = pm.getPreset(i);
                    if (p.category == "USER" && p.file.existsAsFile()) {
                        juce::File dest = targetDir.getChildFile(p.file.getFileName());
                        if (p.file.copyFileTo(dest)) count++;
                    }
                }
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Export Complete",
                    "Exported " + juce::String(count) + " user presets.");
            }
        });
    };

    refresh();
}

void PresetBrowser::refresh()
{
    auto& pm = processor.presetManager;
    pm.refresh();

    categories.clear();
    for (int i = 0; i < pm.getNumPresets(); ++i)
    {
        const auto& cat = pm.getPreset(i).category;
        if (!categories.contains(cat)) categories.add(cat);
    }
    categories.sort(false);

    selCat   = 0;
    selPatch = -1;
    catScroll = patchScroll = 0;
    buildPatches();
    repaint();
}

void PresetBrowser::buildPatches()
{
    patchIndices.clear();
    auto& pm  = processor.presetManager;
    juce::String cat = (selCat >= 0 && selCat < categories.size())
                        ? categories[selCat] : "";
    for (int i = 0; i < pm.getNumPresets(); ++i)
        if (pm.getPreset(i).category == cat)
            patchIndices.push_back(i);
    patchScroll = 0;
    selPatch    = -1;
}

// ── Geometry ─────────────────────────────────────────────────────────────────

void PresetBrowser::resized()
{
    // Panel sits just below the top bar, spanning most of the width
    const int px = 110, py = 56;
    const int pw = juce::jmin(700, getWidth() - px - 20);
    const int ph = 350;
    panel = { px, py, pw, ph };

    int cx = panel.getX() + COL_PAD;
    int cy = panel.getY();
    int ch = panel.getHeight();

    int catW   = 148;
    int infoW  = 160;
    int patchW = pw - catW - infoW - COL_PAD * 4;

    catCol   = { cx,                        cy, catW,   ch - 54 };
    patchCol = { cx + catW + COL_PAD,       cy, patchW, ch };
    infoCol  = { cx + catW + patchW + COL_PAD * 2, cy, infoW, ch };

    newPresetBtn.setBounds(cx, cy + ch - 50, catW, 22);
    exportAllBtn.setBounds(cx, cy + ch - 24, catW, 22);

    closeBtn = { panel.getRight() - 22, panel.getY() + 4, 16, 16 };
}

// ── Row hit-test ─────────────────────────────────────────────────────────────

int PresetBrowser::rowAt(juce::Rectangle<int> col, juce::Point<int> p, int scroll) const
{
    if (!col.contains(p)) return -1;
    int relY = p.getY() - col.getY();   // col already has header trimmed — don't subtract again
    if (relY < 0) return -1;
    return relY / ROW_H + scroll;
}

// ── Drawing ───────────────────────────────────────────────────────────────────

void PresetBrowser::drawColumn(juce::Graphics& g,
                                juce::Rectangle<int> b,
                                const char* title,
                                const juce::StringArray& items,
                                int selected, int hovered, int scroll,
                                juce::Colour accent) const
{
    // Column background
    g.setColour(BG_COL);
    g.fillRect(b);
    g.setColour(BORDER);
    g.drawRect(b, 1);

    // Header
    auto hdr = b.removeFromTop(HDR_H);
    g.setColour(accent.withAlpha(0.18f));
    g.fillRect(hdr);
    g.setColour(accent);
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.5f, juce::Font::bold));
    g.drawText(title, hdr.reduced(COL_PAD, 0), juce::Justification::centredLeft);

    // Clip rows to column body
    g.saveState();
    g.reduceClipRegion(b);

    int visRows = b.getHeight() / ROW_H;
    int end     = juce::jmin(scroll + visRows + 1, items.size());

    for (int i = scroll; i < end; ++i)
    {
        auto row = juce::Rectangle<int>(b.getX(), b.getY() + (i - scroll) * ROW_H, b.getWidth(), ROW_H);
        bool isSel  = (i == selected);
        bool isHov  = (i == hovered);

        if (isSel)
        {
            g.setColour(accent.withAlpha(0.12f));
            g.fillRect(row);
        }

        // Subtle alternating row tint
        if (!isSel && i % 2 == 0)
        {
            g.setColour(juce::Colour(0xff0d0e13));
            g.fillRect(row);
        }

        // Text
        juce::Colour col = isSel ? accent : (isHov ? AMBER : TXT);
        if (isHov && !isSel) col = AMBER;
        g.setColour(col);
        g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 9.0f, juce::Font::plain));
        g.drawText(items[i], row.reduced(COL_PAD, 0), juce::Justification::centredLeft, true);

        // Selected indicator line
        if (isSel)
        {
            g.setColour(accent);
            g.fillRect(row.getX(), row.getY(), 2, row.getHeight());
        }

        // Divider
        g.setColour(juce::Colour(VW::BORDER_SUB));
        g.drawHorizontalLine(row.getBottom() - 1, (float)row.getX() + 2, (float)row.getRight() - 2);
    }

    g.restoreState();
}

void PresetBrowser::paint(juce::Graphics& g)
{
    // Dim overlay behind panel
    g.setColour(juce::Colour(0xaa000000));
    g.fillRect(getLocalBounds());

    // Panel shadow
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.fillRoundedRectangle(panel.toFloat().translated(3, 3), 5.0f);

    // Panel base
    g.setColour(BG);
    g.fillRoundedRectangle(panel.toFloat(), 4.0f);
    g.setColour(AMBER.withAlpha(0.35f));
    g.drawRoundedRectangle(panel.toFloat(), 4.0f, 1.0f);

    // ── Category column ───────────────────────────────────────────────────────
    drawColumn(g, catCol, "CATEGORY", categories, selCat, hovCat, catScroll, AMBER);

    // ── Patch column ─────────────────────────────────────────────────────────
    auto& pm = processor.presetManager;
    juce::StringArray patchNames;
    for (int idx : patchIndices)
        patchNames.add(pm.getPreset(idx).name);

    drawColumn(g, patchCol, "PATCH", patchNames, selPatch, hovPatch, patchScroll, AMBER);

    // ── Info column ───────────────────────────────────────────────────────────
    {
        auto ib = infoCol;
        g.setColour(BG_COL);
        g.fillRect(ib);
        g.setColour(BORDER);
        g.drawRect(ib, 1);

        // Header
        auto hdr = ib.removeFromTop(HDR_H);
        g.setColour(AMBER.withAlpha(0.18f));
        g.fillRect(hdr);
        g.setColour(AMBER);
        g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.5f, juce::Font::bold));
        g.drawText("INFO", hdr.reduced(COL_PAD, 0), juce::Justification::centredLeft);

        if (selPatch >= 0 && selPatch < (int)patchIndices.size())
        {
            const auto& p = pm.getPreset(patchIndices[static_cast<size_t>(selPatch)]);
            int ty = ib.getY() + 8;
            auto tRow = [&](const juce::String& label, const juce::String& val) {
                g.setColour(TXT);
                g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 7.5f, juce::Font::plain));
                g.drawText(label, ib.getX() + COL_PAD, ty, ib.getWidth() - COL_PAD * 2, 11, juce::Justification::centredLeft);
                g.setColour(AMBER.withAlpha(0.85f));
                g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.5f, juce::Font::bold));
                g.drawText(val, ib.getX() + COL_PAD, ty + 11, ib.getWidth() - COL_PAD * 2, 13, juce::Justification::centredLeft);
                ty += 32;
            };
            tRow("NAME",     p.name);
            tRow("CATEGORY", p.category);
            if (p.sub.isNotEmpty()) tRow("TYPE", p.sub);

            // Preset number
            g.setColour(TXT.withAlpha(0.4f));
            g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 7.5f, juce::Font::plain));
            g.drawText("#" + juce::String(patchIndices[static_cast<size_t>(selPatch)] + 1)
                       + " of " + juce::String(pm.getNumPresets()),
                       ib.getX() + COL_PAD, ib.getBottom() - 18,
                       ib.getWidth() - COL_PAD * 2, 12,
                       juce::Justification::centredLeft);
        }
        else
        {
            g.setColour(TXT.withAlpha(0.3f));
            g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.0f, juce::Font::plain));
            g.drawText("select a patch", ib.reduced(COL_PAD, 20), juce::Justification::centred);
        }
    }

    // Close button
    g.setColour(juce::Colour(VW::BG_RAISED));
    g.fillRoundedRectangle(closeBtn.toFloat(), 3.0f);
    g.setColour(TXT);
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 9.0f, juce::Font::bold));
    g.drawText("X", closeBtn, juce::Justification::centred);

    // Scrollbar hint — faint right-edge indicator on patch col if list is long
    int visRows = (patchCol.getHeight() - HDR_H) / ROW_H;
    if ((int)patchIndices.size() > visRows)
    {
        float ratio  = static_cast<float>(visRows) / patchIndices.size();
        float offset = static_cast<float>(patchScroll) / patchIndices.size();
        int   sbH    = static_cast<int>((patchCol.getHeight() - HDR_H) * ratio);
        int   sbY    = patchCol.getY() + HDR_H + static_cast<int>((patchCol.getHeight() - HDR_H) * offset);
        g.setColour(AMBER.withAlpha(0.25f));
        g.fillRoundedRectangle(patchCol.getRight() - 3.0f, (float)sbY, 2.0f, (float)sbH, 1.0f);
    }
}

// ── Mouse ─────────────────────────────────────────────────────────────────────

void PresetBrowser::mouseDown(const juce::MouseEvent& e)
{
    auto pt = e.getPosition();

    // Close button
    if (closeBtn.contains(pt)) { if (onClose) onClose(); return; }

    // Click outside panel → close
    if (!panel.contains(pt))   { if (onClose) onClose(); return; }

    // Category column
    int cr = rowAt(catCol.withTrimmedTop(HDR_H), pt, catScroll);
    if (cr >= 0 && cr < categories.size() && catCol.contains(pt) && pt.getY() > catCol.getY() + HDR_H)
    {
        selCat = cr;
        buildPatches();
        repaint();
        return;
    }

    // Patch column
    int pr = rowAt(patchCol.withTrimmedTop(HDR_H), pt, patchScroll);
    if (pr >= 0 && pr < (int)patchIndices.size() && patchCol.contains(pt) && pt.getY() > patchCol.getY() + HDR_H)
    {
        selPatch = pr;
        repaint();
        int pmIdx = patchIndices[static_cast<size_t>(pr)];
        processor.presetManager.loadPreset(pmIdx);
        if (onPresetSelected) onPresetSelected(pmIdx);
        if (onClose) onClose();
        return;
    }
}

void PresetBrowser::mouseMove(const juce::MouseEvent& e)
{
    auto pt = e.getPosition();
    int oldHovCat   = hovCat;
    int oldHovPatch = hovPatch;

    hovCat   = -1;
    hovPatch = -1;

    if (catCol.contains(pt) && pt.getY() > catCol.getY() + HDR_H)
    {
        int r = rowAt(catCol.withTrimmedTop(HDR_H), pt, catScroll);
        if (r >= 0 && r < categories.size()) hovCat = r;
    }
    if (patchCol.contains(pt) && pt.getY() > patchCol.getY() + HDR_H)
    {
        int r = rowAt(patchCol.withTrimmedTop(HDR_H), pt, patchScroll);
        if (r >= 0 && r < (int)patchIndices.size()) hovPatch = r;
    }

    if (hovCat != oldHovCat || hovPatch != oldHovPatch)
        repaint();
}

void PresetBrowser::mouseExit(const juce::MouseEvent&)
{
    hovCat = hovPatch = -1;
    repaint();
}

void PresetBrowser::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    auto pt = e.getPosition();
    int delta = (w.deltaY > 0) ? -1 : 1;

    if (catCol.contains(pt))
    {
        catScroll = juce::jlimit(0, juce::jmax(0, categories.size() - 1), catScroll + delta);
        repaint();
    }
    else if (patchCol.contains(pt))
    {
        int visRows = (patchCol.getHeight() - HDR_H) / ROW_H;
        patchScroll = juce::jlimit(0, juce::jmax(0, (int)patchIndices.size() - visRows), patchScroll + delta);
        repaint();
    }
}

// ── Keyboard ─────────────────────────────────────────────────────────────────

bool PresetBrowser::keyPressed(const juce::KeyPress& k, juce::Component*)
{
    if (k == juce::KeyPress::escapeKey)
    {
        if (onClose) onClose();
        return true;
    }

    auto& pm = processor.presetManager;
    int visRows = (patchCol.getHeight() - HDR_H) / ROW_H;
    int n = static_cast<int>(patchIndices.size());

    if (k == juce::KeyPress::upKey && n > 0)
    {
        selPatch = juce::jmax(0, selPatch <= 0 ? n - 1 : selPatch - 1);
        patchScroll = juce::jlimit(0, juce::jmax(0, n - visRows), selPatch - visRows / 2);
        repaint();
        return true;
    }
    if (k == juce::KeyPress::downKey && n > 0)
    {
        selPatch = (selPatch + 1) % n;
        patchScroll = juce::jlimit(0, juce::jmax(0, n - visRows), selPatch - visRows / 2);
        repaint();
        return true;
    }
    if (k == juce::KeyPress::returnKey && selPatch >= 0 && selPatch < n)
    {
        int pmIdx = patchIndices[static_cast<size_t>(selPatch)];
        pm.loadPreset(pmIdx);
        if (onPresetSelected) onPresetSelected(pmIdx);
        if (onClose) onClose();
        return true;
    }
    return false;
}
