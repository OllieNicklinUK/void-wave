#include "PluginEditor.h"

static const juce::Colour AMBER { VW::NEON_AMBER };

static void styleCombo(juce::ComboBox& c)
{
    c.setColour(juce::ComboBox::backgroundColourId, juce::Colour(VW::BG_RAISED));
    c.setColour(juce::ComboBox::textColourId,       juce::Colour(VW::TEXT_BRIGHT));
    c.setColour(juce::ComboBox::outlineColourId,    juce::Colour(VW::BORDER_VIS));
    c.setColour(juce::ComboBox::arrowColourId,      AMBER);
    c.setColour(juce::PopupMenu::backgroundColourId,          juce::Colour(VW::BG_RAISED));
    c.setColour(juce::PopupMenu::textColourId,                juce::Colour(VW::TEXT_BRIGHT));
    c.setColour(juce::PopupMenu::highlightedBackgroundColourId, AMBER.withAlpha(0.25f));
    c.setColour(juce::PopupMenu::highlightedTextColourId,     AMBER);
}

// ── Constructor ───────────────────────────────────────────────────────────────

VoidWaveAudioProcessorEditor::VoidWaveAudioProcessorEditor(VoidWaveAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
    , osc1Section      (p, "osc1", AMBER)
    , osc2Section      (p, "osc2", AMBER)
    , wtDisplay        (p)
    , filterSection    (p)
    , envSection       (p)
    , midiLearnOverlay (p, *this)
    , modMatrix    (p)
    , lfoSection   (p)
    , macroSection (p)
    , fxSection    (p)
{
    setLookAndFeel(&lookAndFeel);
    setSize(1100, 620);

    addAndMakeVisible(osc1Section);
    addAndMakeVisible(osc2Section);
    addAndMakeVisible(wtDisplay);
    addAndMakeVisible(filterSection);
    addAndMakeVisible(envSection);
    addAndMakeVisible(modMatrix);
    addAndMakeVisible(lfoSection);
    addAndMakeVisible(macroSection);
    addAndMakeVisible(fxSection);

    // ── Preset bar ──────────────────────────────────────────────────────
    auto styleNavBtn = [](juce::TextButton& b) {
        b.setColour(juce::TextButton::buttonColourId,  juce::Colour(VW::BORDER_VIS));
        b.setColour(juce::TextButton::textColourOffId, AMBER);
    };
    styleNavBtn(prevBtn); styleNavBtn(nextBtn);
    prevBtn.onClick = [this] {
        audioProcessor.presetManager.previousPreset();
        updatePresetDisplay();
    };
    nextBtn.onClick = [this] {
        audioProcessor.presetManager.nextPreset();
        updatePresetDisplay();
    };
    addAndMakeVisible(prevBtn);
    addAndMakeVisible(nextBtn);

    // Category combo
    styleCombo(catCombo);
    catCombo.onChange = [this] { onCategoryChanged(); };
    addAndMakeVisible(catCombo);

    // Preset combo
    styleCombo(presetCombo);
    presetCombo.setTextWhenNothingSelected("--- select preset ---");
    presetCombo.onChange = [this] { onPresetSelected(); };
    addAndMakeVisible(presetCombo);

    // IMPORT button — opens folder picker, loads all .vwpreset files found
    importBtn.setColour(juce::TextButton::buttonColourId,  juce::Colour(VW::BORDER_VIS));
    importBtn.setColour(juce::TextButton::textColourOffId, AMBER);
    importBtn.onClick = [this]
    {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Select a .vwpreset file or a preset folder",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
            "*.vwpreset", true);

        fileChooser->launchAsync(
            juce::FileBrowserComponent::openMode |
            juce::FileBrowserComponent::canSelectFiles |
            juce::FileBrowserComponent::canSelectDirectories |
            juce::FileBrowserComponent::canSelectMultipleItems,
            [this](const juce::FileChooser& fc)
            {
                auto results = fc.getResults();
                if (results.isEmpty()) return;

                int added = 0;
                juce::StringArray foldersImported;

                for (const auto& chosen : results)
                {
                    if (!chosen.exists()) continue;

                    if (chosen.isDirectory())
                    {
                        // Import whole folder tree
                        int n = audioProcessor.presetManager.importFromFolder(chosen);
                        added += n;
                        foldersImported.addIfNotAlreadyThere(chosen.getFullPathName());
                    }
                    else if (chosen.hasFileExtension(".vwpreset"))
                    {
                        // Single file — add directly; category from parent folder name
                        int n = audioProcessor.presetManager.importFromFolder(
                                    chosen.getParentDirectory());
                        added += n;
                        foldersImported.addIfNotAlreadyThere(
                            chosen.getParentDirectory().getFullPathName());
                    }
                }

                populatePresetCombos();

                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::InfoIcon, "Import complete",
                    juce::String(added) + " presets loaded from "
                    + juce::String(foldersImported.size()) + " folder(s).");
            });
    };
    addAndMakeVisible(importBtn);

    // Status
    statusLabel.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.0f, juce::Font::plain));
    statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xff3a3d48));
    statusLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(statusLabel);

    // MIDI MAP button
    midiMapBtn.setClickingTogglesState(true);
    midiMapBtn.setColour(juce::TextButton::buttonColourId,   juce::Colour(VW::BORDER_VIS));
    midiMapBtn.setColour(juce::TextButton::buttonOnColourId, AMBER.withAlpha(0.25f));
    midiMapBtn.setColour(juce::TextButton::textColourOffId,  juce::Colour(VW::TEXT_MID));
    midiMapBtn.setColour(juce::TextButton::textColourOnId,   AMBER);
    midiMapBtn.onClick = [this]
    {
        bool active = midiMapBtn.getToggleState();
        audioProcessor.midiLearnActive = active;
        midiLearnOverlay.setActive(active);
        midiLearnOverlay.setVisible(active);
        midiLearnOverlay.repaint();
    };
    addAndMakeVisible(midiMapBtn);

    // MIDI learn overlay (covers full editor, transparent to mouse, draws highlights)
    midiLearnOverlay.setVisible(false);
    addAndMakeVisible(midiLearnOverlay);

    // Auto-play
    autoPlayBtn.setColour(juce::TextButton::buttonColourId,  juce::Colour(VW::BORDER_VIS));
    autoPlayBtn.setColour(juce::TextButton::textColourOffId, AMBER);
    autoPlayBtn.onClick = [this] {
        audioProcessor.autoPlayEnabled = !audioProcessor.autoPlayEnabled;
        if (!audioProcessor.autoPlayEnabled) audioProcessor.allNotesOff();
    };
    addAndMakeVisible(autoPlayBtn);

    noteLabel.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::bold));
    noteLabel.setColour(juce::Label::textColourId, AMBER);
    noteLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(noteLabel);

    // Build combos and load first preset
    populatePresetCombos();
    audioProcessor.presetManager.loadPreset(0);
    updatePresetDisplay();

    startTimerHz(30);
}

VoidWaveAudioProcessorEditor::~VoidWaveAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

// ── Preset combo logic ────────────────────────────────────────────────────────

void VoidWaveAudioProcessorEditor::populatePresetCombos()
{
    auto& pm = audioProcessor.presetManager;
    pm.refresh();
    int   n  = pm.getNumPresets();


    // Gather unique categories in order of appearance
    juce::StringArray cats;
    cats.add("ALL");
    for (int i = 0; i < n; ++i)
    {
        const auto& cat = pm.getPreset(i).category;
        if (!cats.contains(cat)) cats.add(cat);
    }

    catCombo.clear(juce::dontSendNotification);
    for (int i = 0; i < cats.size(); ++i)
        catCombo.addItem(cats[i], i + 1);
    catCombo.setSelectedId(1, juce::dontSendNotification);   // "ALL"

    // Fill preset combo with all presets initially
    presetCombo.clear(juce::dontSendNotification);
    presetComboMap.clear();
    for (int i = 0; i < n; ++i)
    {
        presetCombo.addItem(pm.getPreset(i).name, i + 1);
        presetComboMap.add(i);
    }

    statusLabel.setText(juce::String(n) + " presets  |  " + juce::String(cats.size() - 1) + " cats", juce::dontSendNotification);
}

void VoidWaveAudioProcessorEditor::onCategoryChanged()
{
    auto& pm      = audioProcessor.presetManager;
    int   n       = pm.getNumPresets();
    juce::String selCat = catCombo.getText();

    presetCombo.clear(juce::dontSendNotification);
    presetComboMap.clear();

    juce::String lastSub;
    for (int i = 0; i < n; ++i)
    {
        const auto& p = pm.getPreset(i);
        if (selCat != "ALL" && p.category != selCat) continue;

        // Sub-category section header when sub changes
        if (p.sub.isNotEmpty() && p.sub != lastSub)
        {
            presetCombo.addSectionHeading(p.sub);
            lastSub = p.sub;
        }

        presetCombo.addItem(p.name, presetComboMap.size() + 1);
        presetComboMap.add(i);
    }

    if (presetCombo.getNumItems() > 0)
    {
        presetCombo.setSelectedId(1, juce::dontSendNotification);
        audioProcessor.presetManager.loadPreset(presetComboMap[0]);
    }
    statusLabel.setText(juce::String(presetComboMap.size()) + " presets", juce::dontSendNotification);
}

void VoidWaveAudioProcessorEditor::onPresetSelected()
{
    int comboId = presetCombo.getSelectedId();
    if (comboId <= 0 || comboId > presetComboMap.size()) return;
    int pmIdx = presetComboMap[comboId - 1];
    audioProcessor.presetManager.loadPreset(pmIdx);
}

void VoidWaveAudioProcessorEditor::updatePresetDisplay()
{
    auto& pm  = audioProcessor.presetManager;
    int   cur = pm.getCurrentIndex();
    if (pm.getNumPresets() == 0) return;

    const auto& p = pm.getPreset(cur);

    // Sync category combo (switch to ALL or correct cat if visible)
    juce::String currentCat = catCombo.getText();
    if (currentCat != "ALL" && currentCat != p.category)
    {
        // Switch to ALL to ensure the preset is visible
        catCombo.setSelectedItemIndex(0, juce::dontSendNotification);
        onCategoryChanged();
    }

    // Sync preset combo to current preset
    for (int i = 0; i < presetComboMap.size(); ++i)
    {
        if (presetComboMap[i] == cur)
        {
            presetCombo.setSelectedId(i + 1, juce::dontSendNotification);
            break;
        }
    }
}

// ── Timer / utils ─────────────────────────────────────────────────────────────

juce::String VoidWaveAudioProcessorEditor::midiNoteToName(int note)
{
    if (note < 0) return "---";
    static const char* names[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
    return juce::String(names[note % 12]) + juce::String(note / 12 - 1);
}

void VoidWaveAudioProcessorEditor::timerCallback()
{
    noteLabel.setText(midiNoteToName(audioProcessor.autoPlayCurNote),
                      juce::dontSendNotification);
}

// ── Paint / layout ────────────────────────────────────────────────────────────

void VoidWaveAudioProcessorEditor::paint(juce::Graphics& g)
{
    const int W = getWidth(), H = getHeight();
    const float fw = static_cast<float>(W), fh = static_cast<float>(H);

    // ── Deep background ────────────────────────────────────────────────────────
    g.fillAll(juce::Colour(VW::BG_DEEP));

    // Ambient blob 1 — cyan tint, top-left
    {
        juce::ColourGradient blob(juce::Colour(VW::NEON_CYAN).withAlpha(0.045f),
                                   fw * 0.08f, fh * 0.28f,
                                   juce::Colours::transparentBlack,
                                   fw * 0.48f, fh * 0.28f, true);
        g.setGradientFill(blob);
        g.fillRect(0, 0, W, H);
    }
    // Ambient blob 2 — violet tint, bottom-right
    {
        juce::ColourGradient blob(juce::Colour(VW::NEON_VIOLET).withAlpha(0.035f),
                                   fw * 0.85f, fh * 0.72f,
                                   juce::Colours::transparentBlack,
                                   fw * 0.55f, fh * 0.72f, true);
        g.setGradientFill(blob);
        g.fillRect(0, 0, W, H);
    }

    // Subtle scanlines — every 2px
    g.setColour(juce::Colour(0x0f000000));
    for (int y = 0; y < H; y += 2)
        g.drawHorizontalLine(y, 0.0f, fw);

    // ── Top bar ────────────────────────────────────────────────────────────────
    {
        juce::ColourGradient tbg(juce::Colour(0xff0d1020), 0.0f, 0.0f,
                                  juce::Colour(VW::BG_PANEL), 0.0f, 56.0f, false);
        g.setGradientFill(tbg);
        g.fillRect(0, 0, W, 56);
    }
    // Top bar bottom border with subtle amber glow
    g.setColour(juce::Colour(VW::BORDER_VIS));
    g.drawHorizontalLine(55, 0.0f, fw);
    g.setColour(AMBER.withAlpha(0.06f));
    g.fillRect(0, 54, W, 2);

    // ── Logo ───────────────────────────────────────────────────────────────────
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 18.0f, juce::Font::bold));
    // "VOID" — white with slight glow
    g.setColour(juce::Colour(VW::TEXT_BRIGHT).withAlpha(0.9f));
    g.drawText("VOID", 14, 8, 60, 20, juce::Justification::centredLeft);
    // "WAVE" — neon amber with glow
    g.setColour(AMBER.withAlpha(0.25f));
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 20.0f, juce::Font::bold));
    g.drawText("WAVE", 55, 7, 62, 22, juce::Justification::centredLeft);
    g.setColour(AMBER);
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 18.0f, juce::Font::bold));
    g.drawText("WAVE", 56, 8, 60, 20, juce::Justification::centredLeft);

    // ── Section separators ─────────────────────────────────────────────────────
    g.setColour(juce::Colour(VW::BORDER_SUB));
    g.drawVerticalLine(260, 56.0f, 277.0f);
    g.drawVerticalLine(521, 56.0f, 277.0f);
    g.drawHorizontalLine(276, 0.0f, fw);

    const int fW = 270, eW = 375, lW = 295;
    g.drawVerticalLine(fW,           277.0f, fh - 79.0f);
    g.drawVerticalLine(fW + 1 + eW,  277.0f, fh - 79.0f);
    g.drawVerticalLine(fW+1+eW+1+lW, 277.0f, fh - 79.0f);
    g.drawHorizontalLine(H - 79, 0.0f, fw);
}

void VoidWaveAudioProcessorEditor::resized()
{
    const int W = getWidth();

    // Top bar layout
    prevBtn    .setBounds(110,     18, 22, 20);
    catCombo   .setBounds(136,     14, 80, 28);   // category filter
    presetCombo.setBounds(220,     14, 390, 28);  // preset list
    nextBtn    .setBounds(614,     18, 22, 20);
    importBtn  .setBounds(640,     16, 56,  24);  // import folder
    statusLabel.setBounds(700,     22, 100, 12);
    midiMapBtn .setBounds(W - 185, 16, 62,  24);  // MIDI MAP left of AUTO
    autoPlayBtn.setBounds(W - 120, 16, 56,  24);
    noteLabel  .setBounds(W - 62,  12, 54,  28);

    midiLearnOverlay.setBounds(getLocalBounds());

    // Grid layout
    const int TOP = 56, FX_H = 79;
    const int ROW1  = 265;
    const int ROW23 = getHeight() - TOP - 1 - ROW1 - 1 - FX_H;  // combined middle
    const int yFX   = getHeight() - FX_H;
    const int y1    = TOP;
    const int y2    = y1 + ROW1 + 1;

    // Row 1: OSC1 | OSC2 | Wavetable
    osc1Section.setBounds(0,   y1, 260, ROW1);
    osc2Section.setBounds(261, y1, 260, ROW1);
    wtDisplay  .setBounds(522, y1, W - 522, ROW1);

    // Row 2: Filter | Envelope | LFO | Macro  (ModMatrix hidden — moved below FX later)
    const int fW = 270, eW = 375, lW = 295, mW = W - fW - eW - lW - 3;
    filterSection.setBounds(0,           y2, fW, ROW23);
    envSection   .setBounds(fW+1,        y2, eW, ROW23);
    lfoSection   .setBounds(fW+1+eW+1,   y2, lW, ROW23);
    macroSection .setBounds(fW+1+eW+1+lW+1, y2, mW, ROW23);
    modMatrix    .setVisible(false);   // will appear below FX in a future session

    fxSection.setBounds(0, yFX, W, FX_H);
}
