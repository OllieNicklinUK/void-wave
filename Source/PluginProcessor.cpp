#include "PluginProcessor.h"
#include "PluginEditor.h"

VoidWaveAudioProcessor::VoidWaveAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    cacheParamPointers();
    wavetableBank.loadBuiltInTables();
    voiceManager.setModMatrix(&modMatrix);
    voiceManager.setWavetableBank(&wavetableBank);
}

// ── Parameter layout ──────────────────────────────────────────────────────────

juce::AudioProcessorValueTreeState::ParameterLayout VoidWaveAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto makeOscParams = [&](const juce::String& px, const juce::String& nm)
    {
        params.push_back(std::make_unique<juce::AudioParameterInt>(
            px + "_table",      nm + " Table",      0, 63,   0));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            px + "_position",   nm + " Position",   0.0f, 1.0f, 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterInt>(
            px + "_coarse",     nm + " Coarse",     -24, 24,  0));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            px + "_fine",       nm + " Fine",       -100.0f, 100.0f, 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            px + "_level",      nm + " Level",      0.0f, 1.0f, 0.8f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            px + "_pan",        nm + " Pan",        -1.0f, 1.0f, 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            px + "_unison",     nm + " Unison",
            juce::StringArray{"1","2","3","4","6","8"}, 0));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            px + "_detune",     nm + " Detune",     0.0f, 1.0f, 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            px + "_phase_mode", nm + " Phase Mode",
            juce::StringArray{"Free","Fixed","Random"}, 0));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            px + "_wt_mode",    nm + " WT Mode",
            juce::StringArray{"Single","Morph","Scan"}, 0));
    };

    makeOscParams("osc1", "OSC 1");
    makeOscParams("osc2", "OSC 2");

    // Per-oscillator stereo width (0=mono, 1=full stereo spread)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "osc1_width", "OSC1 Width", 0.0f, 1.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "osc2_width", "OSC2 Width", 0.0f, 1.0f, 1.0f));

    // Pitch envelope — hardwired to both oscillators (attack + amount)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "pitchenv_attack", "Pitch Env Attack",
        juce::NormalisableRange<float>(0.001f, 0.5f, 0.0f, 0.3f), 0.01f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "pitchenv_amount", "Pitch Env Amount", -24.0f, 24.0f, 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "osc_mix",      "OSC Mix",      0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "osc_mix_mode", "OSC Mix Mode",
        juce::StringArray{"Blend","Ring","Sync","FM"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "osc_fm_ratio", "FM Ratio",     0.5f, 8.0f, 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "filter_type",     "Filter Type",
        juce::StringArray{"LP12","LP24","HP12","HP24","BP","Notch","Comb","Formant"}, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "filter_cutoff",   "Filter Cutoff",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 0.0f, 0.25f), 5000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "filter_res",      "Filter Resonance",  0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "filter_drive",    "Filter Drive",      0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "filter_keytrack", "Key Tracking",      0.0f, 2.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "filter_veltrack", "Vel Tracking",      0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        "filter_route", "Filter Route",         0, 2, 0));  // 0=Both, 1=OSC1, 2=OSC2

    // Sub oscillator + noise
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "sub_level",   "Sub Level",   0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        "sub_octave",  "Sub Octave",  0, 1,   0));   // 0 = -1 oct, 1 = -2 oct
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "noise_level", "Noise Level", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "noise_color", "Noise Color", 0.0f, 1.0f, 0.5f));  // 0=dark LP, 1=white

    auto makeEnvParams = [&](const juce::String& px, const juce::String& nm, bool hasDepth)
    {
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            px + "_attack",   nm + " Attack",
            juce::NormalisableRange<float>(0.001f, 10.0f, 0.0f, 0.3f), 0.01f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            px + "_decay",    nm + " Decay",
            juce::NormalisableRange<float>(0.001f, 10.0f, 0.0f, 0.3f), 0.1f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            px + "_sustain",  nm + " Sustain",   0.0f, 1.0f, 0.7f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            px + "_hold",     nm + " Hold",
            juce::NormalisableRange<float>(0.0f, 5.0f, 0.0f, 0.3f), 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            px + "_release",  nm + " Release",
            juce::NormalisableRange<float>(0.001f, 15.0f, 0.0f, 0.3f), 0.3f));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            px + "_curve",    nm + " Curve",
            juce::StringArray{"Linear","Exp","Log"}, 1));
        if (hasDepth)
            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                px + "_depth",  nm + " Depth",   -1.0f, 1.0f, 0.5f));
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            px + "_loop",     nm + " Loop",      false));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            px + "_vel_sens", nm + " Vel Sens",  0.0f, 1.0f, 0.5f));
    };

    makeEnvParams("env1", "ENV 1", true);
    makeEnvParams("env2", "ENV 2", false);
    makeEnvParams("env3", "ENV 3", false);

    auto makeLfoParams = [&](const juce::String& px, const juce::String& nm)
    {
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            px + "_shape",      nm + " Shape",
            juce::StringArray{"Sine","Tri","Saw","Ramp","Square","S&H","Smooth Rand"}, 0));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            px + "_rate",       nm + " Rate",
            juce::NormalisableRange<float>(0.01f, 20.0f, 0.0f, 0.4f), 1.0f));
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            px + "_tempo_sync", nm + " Tempo Sync",  false));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            px + "_sync_div",   nm + " Sync Div",
            juce::NormalisableRange<float>(0.03125f, 8.0f, 0.0f, 0.5f), 0.5f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            px + "_phase",      nm + " Phase",        0.0f, 360.0f, 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            px + "_trigger",    nm + " Trigger",
            juce::StringArray{"Free","Retrig","One Shot"}, 0));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            px + "_fade",       nm + " Fade In",
            juce::NormalisableRange<float>(0.0f, 10.0f, 0.0f, 0.3f), 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            px + "_depth",      nm + " Depth",       -1.0f, 1.0f, 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            px + "_target",     nm + " Target",
            juce::StringArray{"None","WT Position","Blend (Osc Mix)","Filter Cutoff","Other LFO Rate","Filter Env Amount","Amp Env Attack"}, 0));
    };

    makeLfoParams("lfo1", "LFO 1");
    makeLfoParams("lfo2", "LFO 2");

    for (int i = 1; i <= 12; ++i)
    {
        juce::String px = "mod" + juce::String(i);
        juce::String nm = "Mod " + juce::String(i);
        params.push_back(std::make_unique<juce::AudioParameterInt>(
            px + "_source",  nm + " Source",  0, 14, 0));
        params.push_back(std::make_unique<juce::AudioParameterInt>(
            px + "_dest",    nm + " Dest",    0, 18, 0));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            px + "_depth",   nm + " Depth",  -1.0f, 1.0f, 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            px + "_enabled", nm + " Enabled", false));
    }

    for (int i = 1; i <= 4; ++i)
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            "macro" + juce::String(i), "Macro " + juce::String(i), 0.0f, 1.0f, 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterInt>(
        "voice_poly",       "Polyphony",       1, 16, 8));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "voice_glide",      "Glide",
        juce::NormalisableRange<float>(0.0f, 5.0f, 0.0f, 0.3f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "voice_glide_mode", "Glide Mode",
        juce::StringArray{"Always","Legato"}, 1));
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        "voice_pitch_bend", "Pitch Bend Range", 1, 24, 2));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "master_tune",      "Master Tune",     -100.0f, 100.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "master_volume",    "Master Volume",    0.0f, 1.0f, 0.8f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "fx1_type",   "FX1 Type",
        juce::StringArray{"Soft Clip","Hard Clip","Tube Sat","Foldback","Bitcrusher"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "fx1_param1", "FX1 Drive",  0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "fx1_param2", "FX1 Tone",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 0.0f, 0.25f), 8000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "fx1_param3", "FX1 Mix",   0.0f, 1.0f, 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "fx2_type",   "FX2 Type",
        juce::StringArray{"Chorus","Ensemble","Flanger","Phaser"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "fx2_param1", "FX2 Rate",
        juce::NormalisableRange<float>(0.01f, 20.0f, 0.0f, 0.4f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "fx2_param2", "FX2 Depth",    0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "fx2_param3", "FX2 Mix",      0.0f, 1.0f, 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "fx3_type",   "FX3 Type",
        juce::StringArray{"Mono","Ping-Pong","Stereo"}, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "fx3_param1", "FX3 Time",
        juce::NormalisableRange<float>(1.0f, 2000.0f, 0.0f, 0.3f), 375.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "fx3_param2", "FX3 Feedback", 0.0f, 1.0f, 0.4f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "fx3_param3", "FX3 Mix",      0.0f, 1.0f, 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "fx3_sync",   "FX3 Sync",     false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "fx4_param1", "FX4 Size",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "fx4_param2", "FX4 Damping",  0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "fx4_param3", "FX4 Predelay",
        juce::NormalisableRange<float>(0.0f, 200.0f, 0.0f, 0.5f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "fx4_param4", "FX4 Mix",      0.0f, 1.0f, 0.3f));

    return { params.begin(), params.end() };
}

// ── Param pointer cache ───────────────────────────────────────────────────────

void VoidWaveAudioProcessor::cacheParamPointers()
{
    auto* t = &apvts;
    p_osc1_table      = t->getRawParameterValue("osc1_table");
    p_osc1_position   = t->getRawParameterValue("osc1_position");
    p_osc1_coarse     = t->getRawParameterValue("osc1_coarse");
    p_osc1_fine       = t->getRawParameterValue("osc1_fine");
    p_osc1_level      = t->getRawParameterValue("osc1_level");
    p_osc1_pan        = t->getRawParameterValue("osc1_pan");
    p_osc1_unison     = t->getRawParameterValue("osc1_unison");
    p_osc1_detune     = t->getRawParameterValue("osc1_detune");
    p_osc1_phase_mode = t->getRawParameterValue("osc1_phase_mode");
    p_osc1_wt_mode    = t->getRawParameterValue("osc1_wt_mode");

    p_osc2_table      = t->getRawParameterValue("osc2_table");
    p_osc2_position   = t->getRawParameterValue("osc2_position");
    p_osc2_coarse     = t->getRawParameterValue("osc2_coarse");
    p_osc2_fine       = t->getRawParameterValue("osc2_fine");
    p_osc2_level      = t->getRawParameterValue("osc2_level");
    p_osc2_pan        = t->getRawParameterValue("osc2_pan");
    p_osc2_unison     = t->getRawParameterValue("osc2_unison");
    p_osc2_detune     = t->getRawParameterValue("osc2_detune");
    p_osc2_phase_mode = t->getRawParameterValue("osc2_phase_mode");
    p_osc2_wt_mode    = t->getRawParameterValue("osc2_wt_mode");

    p_osc1_width      = t->getRawParameterValue("osc1_width");
    p_osc2_width      = t->getRawParameterValue("osc2_width");
    p_pitchenv_attack = t->getRawParameterValue("pitchenv_attack");
    p_pitchenv_amount = t->getRawParameterValue("pitchenv_amount");
    p_osc_mix         = t->getRawParameterValue("osc_mix");
    p_osc_mix_mode    = t->getRawParameterValue("osc_mix_mode");
    p_osc_fm_ratio    = t->getRawParameterValue("osc_fm_ratio");

    p_filter_type     = t->getRawParameterValue("filter_type");
    p_filter_cutoff   = t->getRawParameterValue("filter_cutoff");
    p_filter_res      = t->getRawParameterValue("filter_res");
    p_filter_drive    = t->getRawParameterValue("filter_drive");
    p_filter_keytrack = t->getRawParameterValue("filter_keytrack");
    p_filter_veltrack = t->getRawParameterValue("filter_veltrack");
    p_filter_route    = t->getRawParameterValue("filter_route");

    p_sub_level       = t->getRawParameterValue("sub_level");
    p_sub_octave      = t->getRawParameterValue("sub_octave");
    p_noise_level     = t->getRawParameterValue("noise_level");
    p_noise_color     = t->getRawParameterValue("noise_color");

    p_env1_attack     = t->getRawParameterValue("env1_attack");
    p_env1_decay      = t->getRawParameterValue("env1_decay");
    p_env1_sustain    = t->getRawParameterValue("env1_sustain");
    p_env1_hold       = t->getRawParameterValue("env1_hold");
    p_env1_release    = t->getRawParameterValue("env1_release");
    p_env1_curve      = t->getRawParameterValue("env1_curve");
    p_env1_depth      = t->getRawParameterValue("env1_depth");
    p_env1_loop       = t->getRawParameterValue("env1_loop");
    p_env1_vel_sens   = t->getRawParameterValue("env1_vel_sens");

    p_env2_attack     = t->getRawParameterValue("env2_attack");
    p_env2_decay      = t->getRawParameterValue("env2_decay");
    p_env2_sustain    = t->getRawParameterValue("env2_sustain");
    p_env2_hold       = t->getRawParameterValue("env2_hold");
    p_env2_release    = t->getRawParameterValue("env2_release");
    p_env2_curve      = t->getRawParameterValue("env2_curve");
    p_env2_loop       = t->getRawParameterValue("env2_loop");
    p_env2_vel_sens   = t->getRawParameterValue("env2_vel_sens");

    p_env3_attack     = t->getRawParameterValue("env3_attack");
    p_env3_decay      = t->getRawParameterValue("env3_decay");
    p_env3_sustain    = t->getRawParameterValue("env3_sustain");
    p_env3_hold       = t->getRawParameterValue("env3_hold");
    p_env3_release    = t->getRawParameterValue("env3_release");
    p_env3_curve      = t->getRawParameterValue("env3_curve");
    p_env3_loop       = t->getRawParameterValue("env3_loop");
    p_env3_vel_sens   = t->getRawParameterValue("env3_vel_sens");

    p_lfo1_shape      = t->getRawParameterValue("lfo1_shape");
    p_lfo1_rate       = t->getRawParameterValue("lfo1_rate");
    p_lfo1_tempo_sync = t->getRawParameterValue("lfo1_tempo_sync");
    p_lfo1_sync_div   = t->getRawParameterValue("lfo1_sync_div");
    p_lfo1_phase      = t->getRawParameterValue("lfo1_phase");
    p_lfo1_trigger    = t->getRawParameterValue("lfo1_trigger");
    p_lfo1_fade       = t->getRawParameterValue("lfo1_fade");
    p_lfo1_depth      = t->getRawParameterValue("lfo1_depth");
    p_lfo1_target     = t->getRawParameterValue("lfo1_target");

    p_lfo2_shape      = t->getRawParameterValue("lfo2_shape");
    p_lfo2_rate       = t->getRawParameterValue("lfo2_rate");
    p_lfo2_tempo_sync = t->getRawParameterValue("lfo2_tempo_sync");
    p_lfo2_sync_div   = t->getRawParameterValue("lfo2_sync_div");
    p_lfo2_phase      = t->getRawParameterValue("lfo2_phase");
    p_lfo2_trigger    = t->getRawParameterValue("lfo2_trigger");
    p_lfo2_fade       = t->getRawParameterValue("lfo2_fade");
    p_lfo2_depth      = t->getRawParameterValue("lfo2_depth");
    p_lfo2_target     = t->getRawParameterValue("lfo2_target");

    for (int i = 0; i < 12; ++i)
    {
        juce::String px = "mod" + juce::String(i + 1);
        p_mod_source [i] = t->getRawParameterValue(px + "_source");
        p_mod_dest   [i] = t->getRawParameterValue(px + "_dest");
        p_mod_depth  [i] = t->getRawParameterValue(px + "_depth");
        p_mod_enabled[i] = t->getRawParameterValue(px + "_enabled");
    }

    for (int i = 0; i < 4; ++i)
        p_macro[i] = t->getRawParameterValue("macro" + juce::String(i + 1));

    p_voice_poly       = t->getRawParameterValue("voice_poly");
    p_voice_glide      = t->getRawParameterValue("voice_glide");
    p_voice_glide_mode = t->getRawParameterValue("voice_glide_mode");
    p_voice_pitch_bend = t->getRawParameterValue("voice_pitch_bend");
    p_master_tune      = t->getRawParameterValue("master_tune");
    p_master_volume    = t->getRawParameterValue("master_volume");

    p_fx1_type   = t->getRawParameterValue("fx1_type");
    p_fx1_param1 = t->getRawParameterValue("fx1_param1");
    p_fx1_param2 = t->getRawParameterValue("fx1_param2");
    p_fx1_param3 = t->getRawParameterValue("fx1_param3");

    p_fx2_type   = t->getRawParameterValue("fx2_type");
    p_fx2_param1 = t->getRawParameterValue("fx2_param1");
    p_fx2_param2 = t->getRawParameterValue("fx2_param2");
    p_fx2_param3 = t->getRawParameterValue("fx2_param3");

    p_fx3_type   = t->getRawParameterValue("fx3_type");
    p_fx3_param1 = t->getRawParameterValue("fx3_param1");
    p_fx3_param2 = t->getRawParameterValue("fx3_param2");
    p_fx3_param3 = t->getRawParameterValue("fx3_param3");
    p_fx3_sync   = t->getRawParameterValue("fx3_sync");

    p_fx4_param1 = t->getRawParameterValue("fx4_param1");
    p_fx4_param2 = t->getRawParameterValue("fx4_param2");
    p_fx4_param3 = t->getRawParameterValue("fx4_param3");
    p_fx4_param4 = t->getRawParameterValue("fx4_param4");
}

// ── Snapshot builder ─────────────────────────────────────────────────────────

static CurveType intToCurve(int i)
{
    if (i == 1) return CurveType::EXPONENTIAL;
    if (i == 2) return CurveType::LOGARITHMIC;
    return CurveType::LINEAR;
}

SynthParamSnapshot VoidWaveAudioProcessor::buildSnapshot() const
{
    SynthParamSnapshot p;

    p.osc1Table        = static_cast<int>(p_osc1_table->load());
    p.osc1Position     = p_osc1_position->load();
    p.osc1Coarse       = static_cast<int>(p_osc1_coarse->load());
    p.osc1Fine         = p_osc1_fine->load();
    p.osc1Level        = p_osc1_level->load();
    p.osc1Pan          = p_osc1_pan->load();
    p.osc1UnisonVoices = unisonChoiceToCount(static_cast<int>(p_osc1_unison->load()));
    p.osc1Detune       = p_osc1_detune->load();
    p.osc1PhaseMode    = static_cast<PhaseMode>(static_cast<int>(p_osc1_phase_mode->load()));
    p.osc1WTMode       = static_cast<WTMode>   (static_cast<int>(p_osc1_wt_mode->load()));

    p.osc2Table        = static_cast<int>(p_osc2_table->load());
    p.osc2Position     = p_osc2_position->load();
    p.osc2Coarse       = static_cast<int>(p_osc2_coarse->load());
    p.osc2Fine         = p_osc2_fine->load();
    p.osc2Level        = p_osc2_level->load();
    p.osc2Pan          = p_osc2_pan->load();
    p.osc2UnisonVoices = unisonChoiceToCount(static_cast<int>(p_osc2_unison->load()));
    p.osc2Detune       = p_osc2_detune->load();
    p.osc2PhaseMode    = static_cast<PhaseMode>(static_cast<int>(p_osc2_phase_mode->load()));
    p.osc2WTMode       = static_cast<WTMode>   (static_cast<int>(p_osc2_wt_mode->load()));

    p.osc1Width       = p_osc1_width      ? p_osc1_width->load()      : 1.0f;
    p.osc2Width       = p_osc2_width      ? p_osc2_width->load()      : 1.0f;
    p.pitchEnvAttack  = p_pitchenv_attack ? p_pitchenv_attack->load() : 0.01f;
    p.pitchEnvAmount  = p_pitchenv_amount ? p_pitchenv_amount->load() : 0.0f;
    p.oscMix          = p_osc_mix->load();
    p.oscMixMode = static_cast<int>(p_osc_mix_mode->load());
    p.oscFMRatio = p_osc_fm_ratio->load();

    p.filterType     = static_cast<FilterType>(static_cast<int>(p_filter_type->load()));
    p.filterCutoff   = p_filter_cutoff->load();
    p.filterRes      = p_filter_res->load();
    p.filterDrive    = p_filter_drive->load();
    p.filterKeyTrack = p_filter_keytrack->load();
    p.filterVelTrack = p_filter_veltrack->load();
    p.filterRoute    = p_filter_route ? static_cast<int>(p_filter_route->load()) : 0;

    p.subLevel   = p_sub_level   ? p_sub_level->load()                        : 0.0f;
    p.subOctave  = p_sub_octave  ? static_cast<int>(p_sub_octave->load())     : 0;
    p.noiseLevel = p_noise_level ? p_noise_level->load()                      : 0.0f;
    p.noiseColor = p_noise_color ? p_noise_color->load()                      : 0.5f;

    p.env1Attack  = p_env1_attack->load();
    p.env1Decay   = p_env1_decay->load();
    p.env1Sustain = p_env1_sustain->load();
    p.env1Hold    = p_env1_hold->load();
    p.env1Release = p_env1_release->load();
    p.env1Curve   = intToCurve(static_cast<int>(p_env1_curve->load()));
    p.env1Depth   = p_env1_depth->load();
    p.env1Loop    = p_env1_loop->load() > 0.5f;
    p.env1VelSens = p_env1_vel_sens->load();

    p.env2Attack  = p_env2_attack->load();
    p.env2Decay   = p_env2_decay->load();
    p.env2Sustain = p_env2_sustain->load();
    p.env2Hold    = p_env2_hold->load();
    p.env2Release = p_env2_release->load();
    p.env2Curve   = intToCurve(static_cast<int>(p_env2_curve->load()));
    p.env2Loop    = p_env2_loop->load() > 0.5f;
    p.env2VelSens = p_env2_vel_sens->load();

    p.env3Attack  = p_env3_attack->load();
    p.env3Decay   = p_env3_decay->load();
    p.env3Sustain = p_env3_sustain->load();
    p.env3Hold    = p_env3_hold->load();
    p.env3Release = p_env3_release->load();
    p.env3Curve   = intToCurve(static_cast<int>(p_env3_curve->load()));
    p.env3Loop    = p_env3_loop->load() > 0.5f;
    p.env3VelSens = p_env3_vel_sens->load();

    p.lfo1Shape     = static_cast<LFOShape>(static_cast<int>(p_lfo1_shape->load()));
    p.lfo1Rate      = p_lfo1_rate->load();
    p.lfo1TempoSync = p_lfo1_tempo_sync->load() > 0.5f;
    p.lfo1SyncDiv   = p_lfo1_sync_div->load();
    p.lfo1Phase     = p_lfo1_phase->load();
    p.lfo1Trigger   = static_cast<int>(p_lfo1_trigger->load());
    p.lfo1Fade      = p_lfo1_fade->load();
    p.lfo1Depth     = p_lfo1_depth->load();
    p.lfo1Target    = p_lfo1_target ? static_cast<int>(p_lfo1_target->load()) : 0;

    p.lfo2Shape     = static_cast<LFOShape>(static_cast<int>(p_lfo2_shape->load()));
    p.lfo2Rate      = p_lfo2_rate->load();
    p.lfo2TempoSync = p_lfo2_tempo_sync->load() > 0.5f;
    p.lfo2SyncDiv   = p_lfo2_sync_div->load();
    p.lfo2Phase     = p_lfo2_phase->load();
    p.lfo2Trigger   = static_cast<int>(p_lfo2_trigger->load());
    p.lfo2Fade      = p_lfo2_fade->load();
    p.lfo2Depth     = p_lfo2_depth->load();
    p.lfo2Target    = p_lfo2_target ? static_cast<int>(p_lfo2_target->load()) : 0;

    for (int i = 0; i < 4; ++i)
        p.macro[i] = p_macro[i]->load();

    p.masterTune    = p_master_tune->load();
    p.masterVolume  = p_master_volume->load();
    p.hostBPM       = hostBPM;
    p.modWheel      = modWheelValue;
    p.aftertouch    = afterTouchValue;

    return p;
}

// ── FX parameter update ───────────────────────────────────────────────────────

void VoidWaveAudioProcessor::updateFXFromParams()
{
    fxDistortion.setType  (static_cast<DistType>(static_cast<int>(p_fx1_type->load())));
    fxDistortion.setDrive (p_fx1_param1->load());
    fxDistortion.setTone  (p_fx1_param2->load());
    fxDistortion.setMix   (p_fx1_param3->load());

    fxChorus.setType      (static_cast<ModFXType>(static_cast<int>(p_fx2_type->load())));
    fxChorus.setRate      (p_fx2_param1->load());
    fxChorus.setDepth     (p_fx2_param2->load());
    fxChorus.setFeedback  (p_fx2_param2->load() * 0.85f);  // depth doubles as feedback for flanger/phaser
    fxChorus.setMix       (p_fx2_param3->load());

    fxDelay.setType       (static_cast<DelayType>(static_cast<int>(p_fx3_type->load())));
    const bool delaySync   = p_fx3_sync->load() > 0.5f;
    if (delaySync)
        fxDelay.setTempoSync(true, hostBPM, p_fx3_param1->load());
    else
        fxDelay.setTimeMs   (p_fx3_param1->load());
    fxDelay.setFeedback   (p_fx3_param2->load());
    fxDelay.setMix        (p_fx3_param3->load());

    fxReverb.setSize      (p_fx4_param1->load());
    fxReverb.setDamping   (p_fx4_param2->load());
    fxReverb.setPredelay  (p_fx4_param3->load());
    {
        float base  = p_fx4_param4->load();
        float space = p_macro[2] ? p_macro[2]->load() : 0.0f;   // macro3 = SPACE
        fxReverb.setMix(juce::jmin(1.0f, base + space * (1.0f - base)));
    }
}

// ── Dev auto-play ─────────────────────────────────────────────────────────────

void VoidWaveAudioProcessor::tickAutoPlay(int numSamples)
{
    if (!autoPlayEnabled)
    {
        if (autoPlayCurNote >= 0) voiceManager.noteOff(autoPlayCurNote);
        autoPlayCurNote = -1;
        autoPlayCounter = 0;
        autoPlayNoteIdx = 0;
        return;
    }

    // C minor pentatonic across two octaves — good for testing filter, env, and range
    static const int seq[] = { 48, 51, 53, 55, 58, 60, 63, 65, 67, 70, 72, 67, 63, 60, 58, 55 };
    static constexpr int SEQ_LEN = 16;

    autoPlayCounter += numSamples;

    // Note off at 80% of step
    const int stepSamples    = static_cast<int>(getSampleRate() * 0.35);   // ~170bpm 8th notes
    const int noteOnSamples  = static_cast<int>(stepSamples * 0.78);

    if (autoPlayCurNote >= 0 && autoPlayCounter >= noteOnSamples)
    {
        voiceManager.noteOff(autoPlayCurNote);
        autoPlayCurNote = -1;
    }

    if (autoPlayCounter >= stepSamples)
    {
        autoPlayCounter = 0;
        autoPlayCurNote = seq[autoPlayNoteIdx % SEQ_LEN];
        autoPlayNoteIdx = (autoPlayNoteIdx + 1) % SEQ_LEN;
        voiceManager.noteOn(autoPlayCurNote, 0.75f);
    }
}

// ── AudioProcessor overrides ──────────────────────────────────────────────────

void VoidWaveAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    voiceManager.prepare(sampleRate, samplesPerBlock);
    fxDistortion.prepare(sampleRate, samplesPerBlock);
    fxChorus    .prepare(sampleRate, samplesPerBlock);
    fxDelay     .prepare(sampleRate, samplesPerBlock);
    fxReverb    .prepare(sampleRate, samplesPerBlock);

    voiceManager.setMaxVoices(static_cast<int>(p_voice_poly->load()));
    voiceManager.setGlideTime(p_voice_glide->load());
    voiceManager.setGlideMode(p_voice_glide_mode->load() > 0.5f ? GlideMode::LEGATO_ONLY : GlideMode::ALWAYS);
    pitchBendRange = static_cast<float>(static_cast<int>(p_voice_pitch_bend->load()));

    // 120 BPM 8th notes: step = 0.25 s, note-on = 80%, gap = 20%
    int stepSamples    = static_cast<int>(sampleRate * 0.25);
    autoPlayOnSamples  = static_cast<int>(stepSamples * 0.80);
    autoPlayOffSamples = stepSamples - autoPlayOnSamples;
    autoPlayCounter    = 0;
    autoPlayNoteIdx    = 0;
    autoPlayCurNote    = -1;
}

void VoidWaveAudioProcessor::releaseResources() {}

bool VoidWaveAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void VoidWaveAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // ── Host BPM ──────────────────────────────────────────────────────────
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (pos->getBpm().hasValue())
                hostBPM = *pos->getBpm();

    // ── MIDI ──────────────────────────────────────────────────────────────
    pitchBendRange = static_cast<float>(static_cast<int>(p_voice_pitch_bend->load()));
    voiceManager.setMaxVoices(static_cast<int>(p_voice_poly->load()));
    voiceManager.setGlideTime(p_voice_glide->load());
    voiceManager.setGlideMode(p_voice_glide_mode->load() > 0.5f ? GlideMode::LEGATO_ONLY : GlideMode::ALWAYS);

    for (const auto meta : midiMessages)
    {
        const auto msg = meta.getMessage();
        if (msg.isNoteOn())
            voiceManager.noteOn(msg.getNoteNumber(), msg.getFloatVelocity());
        else if (msg.isNoteOff())
            voiceManager.noteOff(msg.getNoteNumber());
        else if (msg.isPitchWheel())
        {
            float norm  = (msg.getPitchWheelValue() - 8192) / 8192.0f;
            voiceManager.setPitchBend(norm * pitchBendRange);
        }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            voiceManager.allNotesOff();
        else if (msg.isChannelPressure())
            afterTouchValue = static_cast<float>(msg.getChannelPressureValue()) / 127.0f;
        else if (msg.isController())
        {
            const int cc  = msg.getControllerNumber();
            const float v = static_cast<float>(msg.getControllerValue()) / 127.0f;
            if (cc == 1)  modWheelValue = v;
            if (cc == 64) voiceManager.setSustain(v > 0.5f);

            // MIDI learn: capture first incoming CC for the selected control
            if (midiLearnActive && midiLearnTarget.isNotEmpty())
            {
                midiMappings[cc] = midiLearnTarget;
                midiLearnTarget  = {};   // ready for next assignment
                // Notify UI on message thread
                juce::MessageManager::callAsync([this]() {
                    if (auto* ed = dynamic_cast<juce::AudioProcessorEditor*>(getActiveEditor()))
                        ed->repaint();
                });
            }

            // Apply any existing CC→param mapping
            if (!midiMappings[cc].isEmpty())
                applyMidiCC(cc, v);
        }
    }

    // ── Dev auto-play ─────────────────────────────────────────────────────
    tickAutoPlay(buffer.getNumSamples());

    // ── Mod matrix from params ────────────────────────────────────────────
    for (int i = 0; i < 12; ++i)
        modMatrix.setSlot(i,
            static_cast<ModSource>(static_cast<int>(p_mod_source[i]->load())),
            static_cast<ModDest>  (static_cast<int>(p_mod_dest  [i]->load())),
            p_mod_depth  [i]->load(),
            p_mod_enabled[i]->load() > 0.5f);

    // ── Voice params snapshot ─────────────────────────────────────────────
    voiceManager.updateFromParams(buildSnapshot());

    // ── Render voices ─────────────────────────────────────────────────────
    voiceManager.process(buffer);

    // ── FX chain ──────────────────────────────────────────────────────────
    updateFXFromParams();
    fxDistortion.process(buffer);
    fxChorus    .process(buffer);
    fxDelay     .process(buffer);
    fxReverb    .process(buffer);

    // ── Master volume ─────────────────────────────────────────────────────
    buffer.applyGain(p_master_volume->load());
}

juce::AudioProcessorEditor* VoidWaveAudioProcessor::createEditor()
{
    return new VoidWaveAudioProcessorEditor(*this);
}

void VoidWaveAudioProcessor::applyMidiCC(int cc, float normalised)
{
    if (cc < 0 || cc >= 128 || midiMappings[cc].isEmpty()) return;
    if (auto* param = apvts.getParameter(midiMappings[cc]))
        param->setValueNotifyingHost(normalised);
}

void VoidWaveAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    // Embed MIDI mappings as child element
    auto* maps = xml->createNewChildElement("MidiMappings");
    for (int i = 0; i < 128; ++i)
        if (!midiMappings[i].isEmpty())
            maps->setAttribute("cc" + juce::String(i), midiMappings[i]);
    copyXmlToBinary(*xml, destData);
}

void VoidWaveAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
    {
        if (auto* maps = xml->getChildByName("MidiMappings"))
            for (int i = 0; i < 128; ++i)
            {
                juce::String key = "cc" + juce::String(i);
                if (maps->hasAttribute(key))
                    midiMappings[i] = maps->getStringAttribute(key);
            }
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VoidWaveAudioProcessor();
}
