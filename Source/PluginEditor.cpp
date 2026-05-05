#include "PluginEditor.h"
#include "VoidWavePresets.h"

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
    , osc1Section      (p, "osc1", juce::Colour(VW::SEC_OSC1))
    , osc2Section      (p, "osc2", juce::Colour(VW::SEC_OSC2))
    , wtDisplay        (p)
    , filterSection    (p)
    , envSection       (p)
    , midiLearnOverlay (p, *this)
    , modMatrix    (p)
    , lfoSection   (p)
    , macroSection (p)
    , fxSection    (p)
    , presetBrowser(p)
{
    setLookAndFeel(&lookAndFeel);
    setSize(1280, 760);

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
        b.setColour(juce::TextButton::buttonColourId,  juce::Colours::transparentBlack);
        b.setColour(juce::TextButton::buttonOnColourId,juce::Colours::transparentBlack);
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

    // Preset name button — click to open the three-column browser
    presetNameBtn.setColour(juce::TextButton::buttonColourId,   juce::Colours::transparentBlack);
    presetNameBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
    presetNameBtn.setColour(juce::TextButton::textColourOffId,  AMBER);
    presetNameBtn.setColour(juce::TextButton::textColourOnId,   AMBER);
    presetNameBtn.setClickingTogglesState(false);
    presetNameBtn.onClick = [this] { showBrowser(); };
    addAndMakeVisible(presetNameBtn);

    // IMPORT button
    importBtn.setColour(juce::TextButton::buttonColourId,  juce::Colours::black);
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
                        added += audioProcessor.presetManager.importFromFolder(chosen);
                        foldersImported.addIfNotAlreadyThere(chosen.getFullPathName());
                    }
                    else if (chosen.hasFileExtension(".vwpreset"))
                    {
                        added += audioProcessor.presetManager.importFromFolder(chosen.getParentDirectory());
                        foldersImported.addIfNotAlreadyThere(chosen.getParentDirectory().getFullPathName());
                    }
                }
                presetBrowser.refresh();
                updatePresetDisplay();
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::InfoIcon, "Import complete",
                    juce::String(added) + " presets loaded from "
                    + juce::String(foldersImported.size()) + " folder(s).");
            });
    };
    addAndMakeVisible(importBtn);

    // Browser panel — full-editor overlay, hidden until opened.
    // Use addChildComponent (not addAndMakeVisible) so it stays hidden at launch.
    presetBrowser.onPresetSelected = [this](int) { updatePresetDisplay(); };
    presetBrowser.onClose          = [this]      { hideBrowser(); };
    addChildComponent(presetBrowser);

    // MIDI MAP button
    midiMapBtn.setClickingTogglesState(true);
    midiMapBtn.setColour(juce::TextButton::buttonColourId,   juce::Colours::black);
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

    // Save preset button
    saveBtn.setColour(juce::TextButton::buttonColourId,  juce::Colours::black);
    saveBtn.setColour(juce::TextButton::textColourOffId, AMBER);
    saveBtn.onClick = [this] { onSavePreset(); };
    addAndMakeVisible(saveBtn);

    // Auto-play — standalone only (VST/AU has no use for it and causes launch noise)
    const bool isStandalone = audioProcessor.wrapperType == juce::AudioProcessor::wrapperType_Standalone;
    autoPlayBtn.setColour(juce::TextButton::buttonColourId,  juce::Colours::black);
    autoPlayBtn.setColour(juce::TextButton::textColourOffId, AMBER);
    autoPlayBtn.onClick = [this] {
        audioProcessor.autoPlayEnabled = !audioProcessor.autoPlayEnabled;
        if (!audioProcessor.autoPlayEnabled) audioProcessor.allNotesOff();
    };
    autoPlayBtn.setVisible(isStandalone);

    noteLabel.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::bold));
    noteLabel.setColour(juce::Label::textColourId, AMBER);
    noteLabel.setJustificationType(juce::Justification::centredRight);
    noteLabel.setVisible(isStandalone);

    addAndMakeVisible(autoPlayBtn);
    addAndMakeVisible(noteLabel);

    // Build preset list. In VST/AU the DAW already restored state via setStateInformation.
    presetBrowser.refresh();
    if (isStandalone)
        audioProcessor.presetManager.loadPreset(0);
    updatePresetDisplay();

    startTimerHz(30);
}

VoidWaveAudioProcessorEditor::~VoidWaveAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

// ── Preset browser logic ──────────────────────────────────────────────────────

void VoidWaveAudioProcessorEditor::showBrowser()
{
    presetBrowser.setBounds(getLocalBounds());
    presetBrowser.setVisible(true);
    presetBrowser.toFront(true);
    presetBrowser.grabKeyboardFocus();
}

void VoidWaveAudioProcessorEditor::hideBrowser()
{
    presetBrowser.setVisible(false);
    updatePresetDisplay();
}

void VoidWaveAudioProcessorEditor::updatePresetDisplay()
{
    auto& pm = audioProcessor.presetManager;
    if (pm.getNumPresets() == 0) { presetNameBtn.setButtonText("--- no presets ---"); return; }

    const auto& p = pm.getPreset(pm.getCurrentIndex());
    presetNameBtn.setButtonText(p.category + "  /  " + p.name);
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

void VoidWaveAudioProcessorEditor::onSavePreset()
{
    // Show a name-entry dialog, then save current APVTS state as a .vwpreset
    auto dialog = std::make_shared<juce::AlertWindow>("Save Preset", "Enter a name for this preset:", juce::MessageBoxIconType::NoIcon);
    dialog->addTextEditor("name", "", "Preset name");
    dialog->addButton("Save",   1, juce::KeyPress(juce::KeyPress::returnKey));
    dialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    // Suggest current preset name as default
    auto& pm = audioProcessor.presetManager;
    if (pm.getNumPresets() > 0)
        dialog->getTextEditor("name")->setText(pm.getPreset(pm.getCurrentIndex()).name);

    dialog->enterModalState(true, juce::ModalCallbackFunction::create(
        [this, dialog](int result)
        {
            if (result == 0) return;
            juce::String name = dialog->getTextEditorContents("name").trim();
            if (name.isEmpty()) return;

            // Ask which category to save into
            auto& pm2 = audioProcessor.presetManager;
            juce::StringArray cats;
            for (int i = 0; i < pm2.getNumPresets(); ++i)
            {
                const auto& cat = pm2.getPreset(i).category;
                if (!cats.contains(cat)) cats.add(cat);
            }
            if (cats.isEmpty()) cats.add("USER");

            juce::AlertWindow::showOkCancelBox(
                juce::MessageBoxIconType::NoIcon, "Category",
                "Save '" + name + "' to which category?\n\n" + cats.joinIntoString(" / "),
                "USER", "Cancel", nullptr,
                juce::ModalCallbackFunction::create([this, name, cats](int ok)
                {
                    juce::String cat = ok ? "USER" : "";
                    if (cat.isEmpty()) return;

                    if (audioProcessor.presetManager.savePreset(name, cat))
                    {
                        presetBrowser.refresh();
                        updatePresetDisplay();
                    }
                }));
        }));
}

// ── Paint / layout ────────────────────────────────────────────────────────────

void VoidWaveAudioProcessorEditor::paint(juce::Graphics& g)
{
    const int W = getWidth(), H = getHeight();
    const float fw = static_cast<float>(W), fh = static_cast<float>(H);

    {
        auto img = juce::ImageCache::getFromMemory(
            VWPresets::flatui_png, VWPresets::flatui_pngSize);
        if (img.isValid())
            g.drawImage(img, 0, 0, W, H, 0, 0, img.getWidth(), img.getHeight());
        else
            g.fillAll(juce::Colours::black);
    }

    // Logo and separators are in the PNG background — nothing to draw here
}

void VoidWaveAudioProcessorEditor::resized()
{
    const int W = getWidth();

    // Top bar layout
    // Preset group: prevBtn(22) + 4 + presetName(470) + 4 + nextBtn(22) = 522px, centred
    {
        const int gW = 522, gX = (getWidth() - gW) / 2;
        prevBtn      .setBounds(gX,          18, 22,  20);
        presetNameBtn.setBounds(gX + 26,     14, 470, 28);
        nextBtn      .setBounds(gX + 500,    18, 22,  20);
    }
    importBtn    .setBounds(W - 316, 16, 56,  24);  // next to SAVE
    saveBtn      .setBounds(W - 256, 16, 56,  24);
    midiMapBtn   .setBounds(W - 192, 16, 62,  24);
    autoPlayBtn  .setBounds(W - 126, 16, 56,  24);
    noteLabel    .setBounds(W - 66,  12, 58,  28);

    midiLearnOverlay.setBounds(getLocalBounds());
    presetBrowser   .setBounds(getLocalBounds());

    // Grid layout — 1280×760
    const int TOP = 56, FX_H = 84;
    const int ROW1  = 310;
    const int ROW23 = getHeight() - TOP - 1 - ROW1 - 1 - FX_H;
    const int yFX   = getHeight() - FX_H;
    const int y1    = TOP;
    const int y2    = y1 + ROW1 + 1;

    // Row 1: OSC1 (280) | OSC2 (280) | Wavetable (rest)
    osc1Section.setBounds(0,   y1, 280, ROW1);
    osc2Section.setBounds(281, y1, 280, ROW1);
    wtDisplay  .setBounds(562, y1, W - 562, ROW1);

    // Row 2: Filter (290) | Envelope (410) | LFO (330) | Macro (rest)
    const int fW = 290, eW = 410, lW = 330, mW = W - fW - eW - lW - 3;
    filterSection.setBounds(0,           y2, fW, ROW23);
    envSection   .setBounds(fW+1,        y2, eW, ROW23);
    lfoSection   .setBounds(fW+1+eW+1,   y2, lW, ROW23);
    macroSection .setBounds(fW+1+eW+1+lW+1, y2, mW, ROW23);
    modMatrix    .setVisible(false);

    fxSection.setBounds(0, yFX, W, FX_H);
}
