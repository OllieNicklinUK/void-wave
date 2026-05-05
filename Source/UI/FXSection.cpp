#include "FXSection.h"
#include "LookAndFeel.h"
static const juce::Colour ORANGE { VW::SEC_FX };

static void iniMiniKnob(juce::Slider& s, juce::Label& l, juce::Colour acc)
{
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    s.setColour(juce::Slider::thumbColourId,               acc);
    s.setColour(juce::Slider::rotarySliderFillColourId,    acc);
    s.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(VW::BORDER_VIS));
    l.setJustificationType(juce::Justification::centred);
    l.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.0f, juce::Font::plain));
    l.setColour(juce::Label::textColourId, juce::Colour(VW::TEXT_MID));
}
static void iniCombo(juce::ComboBox& c, juce::Colour acc)
{
    c.setColour(juce::ComboBox::backgroundColourId, juce::Colour(VW::BG_RAISED));
    c.setColour(juce::ComboBox::textColourId,       juce::Colour(VW::TEXT_BRIGHT));
    c.setColour(juce::ComboBox::outlineColourId,    juce::Colour(VW::BORDER_VIS));
    c.setColour(juce::ComboBox::arrowColourId,      acc);
}
static void iniLabel(juce::Label& l, const char* text, juce::Colour col)
{
    l.setText(text, juce::dontSendNotification);
    l.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.0f, juce::Font::bold));
    l.setColour(juce::Label::textColourId, col);
}

void FXSection::applyReverbPreset(int idx)
{
    struct Preset { float size, damp, pre, mix; };
    static const Preset presets[] = {
        { 0.3f, 0.65f,  5.0f, 0.25f },  // ROOM
        { 0.75f, 0.30f, 22.0f, 0.40f }, // HALL
        { 0.50f, 0.75f,  0.0f, 0.35f }, // PLATE
    };
    if (idx < 0 || idx > 2) return;
    auto setP = [this](const char* id, float v) {
        if (auto* param = processor.apvts.getParameter(id))
            param->setValueNotifyingHost(param->convertTo0to1(v));
    };
    setP("fx4_param1", presets[idx].size);
    setP("fx4_param2", presets[idx].damp);
    setP("fx4_param3", presets[idx].pre);
    setP("fx4_param4", presets[idx].mix);
}

FXSection::FXSection(VoidWaveAudioProcessor& p) : processor(p)
{
    auto& t = p.apvts;

    iniMiniKnob(sFX1Drive, lFX1Drive, ORANGE);
    iniMiniKnob(sFX1Tone,  lFX1Tone,  ORANGE);
    iniMiniKnob(sFX1Mix,   lFX1Mix,   ORANGE);
    iniMiniKnob(sFX2Rate,  lFX2Rate,  ORANGE);
    iniMiniKnob(sFX2Depth, lFX2Depth, ORANGE);
    iniMiniKnob(sFX2Mix,   lFX2Mix,   ORANGE);
    iniMiniKnob(sFX3Time,  lFX3Time,  ORANGE);
    iniMiniKnob(sFX3Fbk,   lFX3Fbk,   ORANGE);
    iniMiniKnob(sFX3Mix,   lFX3Mix,   ORANGE);
    iniMiniKnob(sFX4Size,  lFX4Size,  ORANGE);
    iniMiniKnob(sFX4Damp,  lFX4Damp,  ORANGE);
    iniMiniKnob(sFX4Pre,   lFX4Pre,   ORANGE);
    iniMiniKnob(sFX4Mix,   lFX4Mix,   ORANGE);

    iniCombo(cFX1Type, ORANGE);
    iniCombo(cFX2Type, ORANGE);
    iniCombo(cFX3Type, ORANGE);
    iniCombo(cFX4Type, ORANGE);

    iniLabel(lFX1, "DISTORTION", ORANGE);
    iniLabel(lFX2, "MODULATION", ORANGE);
    iniLabel(lFX3, "DELAY",      ORANGE);
    iniLabel(lFX4, "REVERB",     ORANGE);

    lFX4Type.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.0f, juce::Font::plain));
    lFX4Type.setColour(juce::Label::textColourId, juce::Colour(VW::TEXT_MID));

    // Reverb type — not in APVTS, sets knob values on change
    cFX4Type.addItem("ROOM",  1);
    cFX4Type.addItem("HALL",  2);
    cFX4Type.addItem("PLATE", 3);
    // Populate APVTS-bound combos from their choice parameter strings
    auto pop = [&](juce::ComboBox& c, const juce::String& id) {
        if (auto* p = dynamic_cast<juce::AudioParameterChoice*>(processor.apvts.getParameter(id)))
            c.addItemList(p->choices, 1);
    };
    pop(cFX1Type, "fx1_type");
    pop(cFX2Type, "fx2_type");
    pop(cFX3Type, "fx3_type");
    // cFX4Type populated manually below (ROOM/HALL/PLATE not in APVTS)

    cFX4Type.setSelectedId(1, juce::dontSendNotification);
    cFX4Type.onChange = [this] { applyReverbPreset(cFX4Type.getSelectedId() - 1); };

    // Delay sync toggle
    tFX3Sync.setClickingTogglesState(true);
    tFX3Sync.setColour(juce::TextButton::buttonColourId,   juce::Colour(VW::BG_RAISED));
    tFX3Sync.setColour(juce::TextButton::buttonOnColourId, ORANGE.withAlpha(0.3f));
    tFX3Sync.setColour(juce::TextButton::textColourOffId,  juce::Colour(VW::TEXT_MID));
    tFX3Sync.setColour(juce::TextButton::textColourOnId,   ORANGE);

    for (auto* s : { &sFX1Drive,&sFX1Tone,&sFX1Mix,&sFX2Rate,&sFX2Depth,&sFX2Mix,
                     &sFX3Time,&sFX3Fbk,&sFX3Mix,&sFX4Size,&sFX4Damp,&sFX4Pre,&sFX4Mix })
        addAndMakeVisible(s);
    for (auto* l : { &lFX1Drive,&lFX1Tone,&lFX1Mix,&lFX2Rate,&lFX2Depth,&lFX2Mix,
                     &lFX3Time,&lFX3Fbk,&lFX3Mix,&lFX4Size,&lFX4Damp,&lFX4Pre,&lFX4Mix,
                     &lFX1,&lFX2,&lFX3,&lFX4,&lFX4Type })
        addAndMakeVisible(l);
    for (auto* c : { &cFX1Type,&cFX2Type,&cFX3Type,&cFX4Type })
        addAndMakeVisible(c);
    addAndMakeVisible(tFX3Sync);

    aFX1Typ  = std::make_unique<CbAtt>(t, "fx1_type",   cFX1Type);
    aFX1Drv  = std::make_unique<SlAtt>(t, "fx1_param1", sFX1Drive);
    aFX1Ton  = std::make_unique<SlAtt>(t, "fx1_param2", sFX1Tone);
    aFX1Mix  = std::make_unique<SlAtt>(t, "fx1_param3", sFX1Mix);
    aFX2Typ  = std::make_unique<CbAtt>(t, "fx2_type",   cFX2Type);
    aFX2Rat  = std::make_unique<SlAtt>(t, "fx2_param1", sFX2Rate);
    aFX2Dep  = std::make_unique<SlAtt>(t, "fx2_param2", sFX2Depth);
    aFX2Mix  = std::make_unique<SlAtt>(t, "fx2_param3", sFX2Mix);
    aFX3Typ  = std::make_unique<CbAtt>(t, "fx3_type",   cFX3Type);
    aFX3Tim  = std::make_unique<SlAtt>(t, "fx3_param1", sFX3Time);
    aFX3Fbk  = std::make_unique<SlAtt>(t, "fx3_param2", sFX3Fbk);
    aFX3Mix  = std::make_unique<SlAtt>(t, "fx3_param3", sFX3Mix);
    aFX3Sync = std::make_unique<BtAtt>(t, "fx3_sync",   tFX3Sync);
    aFX4Siz  = std::make_unique<SlAtt>(t, "fx4_param1", sFX4Size);
    aFX4Dam  = std::make_unique<SlAtt>(t, "fx4_param2", sFX4Damp);
    aFX4Pre  = std::make_unique<SlAtt>(t, "fx4_param3", sFX4Pre);
    aFX4Mix  = std::make_unique<SlAtt>(t, "fx4_param4", sFX4Mix);

    // ComponentIDs for MIDI learn
    sFX1Drive.setComponentID("fx1_param1"); sFX1Tone.setComponentID("fx1_param2");
    sFX1Mix  .setComponentID("fx1_param3");
    sFX2Rate .setComponentID("fx2_param1"); sFX2Depth.setComponentID("fx2_param2");
    sFX2Mix  .setComponentID("fx2_param3");
    sFX3Time .setComponentID("fx3_param1"); sFX3Fbk.setComponentID("fx3_param2");
    sFX3Mix  .setComponentID("fx3_param3");
    sFX4Size .setComponentID("fx4_param1"); sFX4Damp.setComponentID("fx4_param2");
    sFX4Pre  .setComponentID("fx4_param3"); sFX4Mix .setComponentID("fx4_param4");

    applyReverbPreset(0);
}

void FXSection::paint(juce::Graphics&) {}

void FXSection::resized()
{
    const int W = getWidth(), H = getHeight(), pad = 8;
    const int slotW = W / 4, Km = 28;

    auto layoutSlot = [&](int slotIdx, juce::Label& hdr, juce::ComboBox* cb,
                          std::initializer_list<juce::Slider*> ks,
                          std::initializer_list<juce::Label*>  ls,
                          juce::TextButton* syncBtn = nullptr)
    {
        int x = slotIdx * slotW + pad, y = 4, w = slotW - 2 * pad;
        hdr.setBounds(x, y, w, 11); y += 13;
        if (cb) { cb->setBounds(x, y, w - (syncBtn ? 32 : 0), 16);
                  if (syncBtn) syncBtn->setBounds(x + w - 28, y, 28, 16);
                  y += 20; }
        int n = static_cast<int>(ks.size()), kw = n > 0 ? w / n : w;
        int ki = 0;
        auto kit = ks.begin(); auto lit = ls.begin();
        for (; kit != ks.end(); ++kit, ++lit, ++ki)
        {
            int kx = x + ki * kw + (kw - Km) / 2;
            (*kit)->setBounds(kx, y, Km, Km);
            (*lit)->setBounds(x + ki * kw, y + Km + 2, kw, 10);
        }
    };

    // FX4 reverb: type label + combo above knobs
    {
        int x = 3 * slotW + pad, w = slotW - 2 * pad;
        lFX4    .setBounds(x, 4, w, 11);
        lFX4Type.setBounds(x, 17, 22, 10);
        cFX4Type.setBounds(x + 24, 15, w - 24, 16);
        int y = 35, kw = w / 4, Km = 28;
        juce::Slider* ks[] = { &sFX4Size, &sFX4Damp, &sFX4Pre, &sFX4Mix };
        juce::Label*  ls[] = { &lFX4Size, &lFX4Damp, &lFX4Pre, &lFX4Mix };
        for (int i = 0; i < 4; ++i)
        {
            int kx = x + i * kw + (kw - Km) / 2;
            ks[i]->setBounds(kx, y, Km, Km);
            ls[i]->setBounds(x + i * kw, y + Km + 2, kw, 10);
        }
    }

    layoutSlot(0, lFX1, &cFX1Type, {&sFX1Drive,&sFX1Tone,&sFX1Mix}, {&lFX1Drive,&lFX1Tone,&lFX1Mix});
    layoutSlot(1, lFX2, &cFX2Type, {&sFX2Rate,&sFX2Depth,&sFX2Mix},  {&lFX2Rate,&lFX2Depth,&lFX2Mix});
    layoutSlot(2, lFX3, &cFX3Type, {&sFX3Time,&sFX3Fbk,&sFX3Mix},    {&lFX3Time,&lFX3Fbk,&lFX3Mix}, &tFX3Sync);
    juce::ignoreUnused(H);
}
