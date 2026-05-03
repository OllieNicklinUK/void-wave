#pragma once
#include <JuceHeader.h>
#include "DSP/VoiceManager.h"
#include "DSP/ModMatrix.h"
#include "DSP/FXChain/Distortion.h"
#include "DSP/FXChain/Chorus.h"
#include "DSP/FXChain/Delay.h"
#include "DSP/FXChain/Reverb.h"
#include "Data/WavetableBank.h"
#include "Data/PresetManager.h"

class VoidWaveAudioProcessor : public juce::AudioProcessor
{
public:
    VoidWaveAudioProcessor();
    ~VoidWaveAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi()  const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int  getNumPrograms() override                              { return 1; }
    int  getCurrentProgram() override                          { return 0; }
    void setCurrentProgram(int) override                       {}
    const juce::String getProgramName(int) override            { return {}; }
    void changeProgramName(int, const juce::String&) override  {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;
    PresetManager                      presetManager { apvts };

    // Dev auto-play
    bool autoPlayEnabled = true;
    int  autoPlayCurNote = -1;
    void allNotesOff() { voiceManager.allNotesOff(); }

    // ── MIDI Learn ─────────────────────────────────────────────────────────
    bool         midiLearnActive = false;
    juce::String midiLearnTarget;                // paramId waiting for a CC
    juce::String midiMappings[128];              // index=CC, value=paramId (empty=unmapped)

    void applyMidiCC(int cc, float normalised);  // apply a CC to its mapped param
    void clearMidiMapping(int cc)  { midiMappings[cc] = {}; }
    void clearAllMidiMappings()    { for (auto& m : midiMappings) m = {}; }
    int  ccForParam(const juce::String& id) const
    {
        for (int i = 0; i < 128; ++i) if (midiMappings[i] == id) return i;
        return -1;
    }

private:
    // ── DSP ────────────────────────────────────────────────────────────────
    WavetableBank wavetableBank;
    ModMatrix     modMatrix;
    VoiceManager  voiceManager;
    Distortion    fxDistortion;
    Chorus        fxChorus;
    Delay         fxDelay;
    VWReverb      fxReverb;

    double hostBPM         = 120.0;
    float  modWheelValue   = 0.0f;   // CC1,  0–1
    float  afterTouchValue = 0.0f;   // channel AT, 0–1
    float  pitchBendRange = 2.0f;

    // ── Dev auto-play internals ────────────────────────────────────────────
    int  autoPlayCounter    = 0;
    int  autoPlayNoteIdx    = 0;
    int  autoPlayOnSamples  = 0;
    int  autoPlayOffSamples = 0;
    void tickAutoPlay(int numSamples);

    // ── Raw parameter pointer cache (audio-thread safe) ────────────────────
    // OSC 1
    std::atomic<float>* p_osc1_table       = nullptr;
    std::atomic<float>* p_osc1_position    = nullptr;
    std::atomic<float>* p_osc1_coarse      = nullptr;
    std::atomic<float>* p_osc1_fine        = nullptr;
    std::atomic<float>* p_osc1_level       = nullptr;
    std::atomic<float>* p_osc1_pan         = nullptr;
    std::atomic<float>* p_osc1_unison      = nullptr;
    std::atomic<float>* p_osc1_detune      = nullptr;
    std::atomic<float>* p_osc1_phase_mode  = nullptr;
    std::atomic<float>* p_osc1_wt_mode     = nullptr;
    // OSC 2
    std::atomic<float>* p_osc2_table       = nullptr;
    std::atomic<float>* p_osc2_position    = nullptr;
    std::atomic<float>* p_osc2_coarse      = nullptr;
    std::atomic<float>* p_osc2_fine        = nullptr;
    std::atomic<float>* p_osc2_level       = nullptr;
    std::atomic<float>* p_osc2_pan         = nullptr;
    std::atomic<float>* p_osc2_unison      = nullptr;
    std::atomic<float>* p_osc2_detune      = nullptr;
    std::atomic<float>* p_osc2_phase_mode  = nullptr;
    std::atomic<float>* p_osc2_wt_mode     = nullptr;
    // OSC Mix
    std::atomic<float>* p_osc1_width        = nullptr;
    std::atomic<float>* p_osc2_width        = nullptr;
    std::atomic<float>* p_pitchenv_attack   = nullptr;
    std::atomic<float>* p_pitchenv_amount   = nullptr;
    std::atomic<float>* p_osc_mix          = nullptr;
    std::atomic<float>* p_osc_mix_mode     = nullptr;
    std::atomic<float>* p_osc_fm_ratio     = nullptr;
    // Filter
    std::atomic<float>* p_filter_type      = nullptr;
    std::atomic<float>* p_filter_cutoff    = nullptr;
    std::atomic<float>* p_filter_res       = nullptr;
    std::atomic<float>* p_filter_drive     = nullptr;
    std::atomic<float>* p_filter_keytrack  = nullptr;
    std::atomic<float>* p_filter_veltrack  = nullptr;
    // ENV 1
    std::atomic<float>* p_env1_attack      = nullptr;
    std::atomic<float>* p_env1_decay       = nullptr;
    std::atomic<float>* p_env1_sustain     = nullptr;
    std::atomic<float>* p_env1_hold        = nullptr;
    std::atomic<float>* p_env1_release     = nullptr;
    std::atomic<float>* p_env1_curve       = nullptr;
    std::atomic<float>* p_env1_depth       = nullptr;
    std::atomic<float>* p_env1_loop        = nullptr;
    std::atomic<float>* p_env1_vel_sens    = nullptr;
    // ENV 2
    std::atomic<float>* p_env2_attack      = nullptr;
    std::atomic<float>* p_env2_decay       = nullptr;
    std::atomic<float>* p_env2_sustain     = nullptr;
    std::atomic<float>* p_env2_hold        = nullptr;
    std::atomic<float>* p_env2_release     = nullptr;
    std::atomic<float>* p_env2_curve       = nullptr;
    std::atomic<float>* p_env2_loop        = nullptr;
    std::atomic<float>* p_env2_vel_sens    = nullptr;
    // ENV 3
    std::atomic<float>* p_env3_attack      = nullptr;
    std::atomic<float>* p_env3_decay       = nullptr;
    std::atomic<float>* p_env3_sustain     = nullptr;
    std::atomic<float>* p_env3_hold        = nullptr;
    std::atomic<float>* p_env3_release     = nullptr;
    std::atomic<float>* p_env3_curve       = nullptr;
    std::atomic<float>* p_env3_loop        = nullptr;
    std::atomic<float>* p_env3_vel_sens    = nullptr;
    // LFO 1
    std::atomic<float>* p_lfo1_shape       = nullptr;
    std::atomic<float>* p_lfo1_rate        = nullptr;
    std::atomic<float>* p_lfo1_tempo_sync  = nullptr;
    std::atomic<float>* p_lfo1_sync_div    = nullptr;
    std::atomic<float>* p_lfo1_phase       = nullptr;
    std::atomic<float>* p_lfo1_trigger     = nullptr;
    std::atomic<float>* p_lfo1_fade        = nullptr;
    std::atomic<float>* p_lfo1_depth       = nullptr;
    // LFO 2
    std::atomic<float>* p_lfo2_shape       = nullptr;
    std::atomic<float>* p_lfo2_rate        = nullptr;
    std::atomic<float>* p_lfo2_tempo_sync  = nullptr;
    std::atomic<float>* p_lfo2_sync_div    = nullptr;
    std::atomic<float>* p_lfo2_phase       = nullptr;
    std::atomic<float>* p_lfo2_trigger     = nullptr;
    std::atomic<float>* p_lfo2_fade        = nullptr;
    std::atomic<float>* p_lfo2_depth       = nullptr;
    // Mod matrix (12 × 4)
    std::atomic<float>* p_mod_source  [12] = {};
    std::atomic<float>* p_mod_dest    [12] = {};
    std::atomic<float>* p_mod_depth   [12] = {};
    std::atomic<float>* p_mod_enabled [12] = {};
    // Macros
    std::atomic<float>* p_macro[4] = {};
    // Voice / global
    std::atomic<float>* p_voice_poly       = nullptr;
    std::atomic<float>* p_voice_glide      = nullptr;
    std::atomic<float>* p_voice_glide_mode = nullptr;
    std::atomic<float>* p_voice_pitch_bend = nullptr;
    std::atomic<float>* p_master_tune      = nullptr;
    std::atomic<float>* p_master_volume    = nullptr;
    // FX
    std::atomic<float>* p_fx1_type         = nullptr;
    std::atomic<float>* p_fx1_param1       = nullptr;
    std::atomic<float>* p_fx1_param2       = nullptr;
    std::atomic<float>* p_fx1_param3       = nullptr;
    std::atomic<float>* p_fx2_type         = nullptr;
    std::atomic<float>* p_fx2_param1       = nullptr;
    std::atomic<float>* p_fx2_param2       = nullptr;
    std::atomic<float>* p_fx2_param3       = nullptr;
    std::atomic<float>* p_fx3_type         = nullptr;
    std::atomic<float>* p_fx3_param1       = nullptr;
    std::atomic<float>* p_fx3_param2       = nullptr;
    std::atomic<float>* p_fx3_param3       = nullptr;
    std::atomic<float>* p_fx3_sync         = nullptr;
    std::atomic<float>* p_fx4_param1       = nullptr;
    std::atomic<float>* p_fx4_param2       = nullptr;
    std::atomic<float>* p_fx4_param3       = nullptr;
    std::atomic<float>* p_fx4_param4       = nullptr;

    void cacheParamPointers();
    SynthParamSnapshot buildSnapshot() const;
    void updateFXFromParams();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoidWaveAudioProcessor)
};
