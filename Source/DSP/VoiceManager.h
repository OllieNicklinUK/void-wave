#pragma once
#include <JuceHeader.h>
#include "UnisonVoice.h"
#include "Filter.h"
#include "EnvelopeGenerator.h"
#include "LFO.h"
#include "ModMatrix.h"
#include "../Data/WavetableBank.h"

// Snapshot of all synth parameters read from APVTS atomics once per block.
// Passed to VoiceManager::updateFromParams() on the audio thread.
struct SynthParamSnapshot
{
    // ── OSC 1 ──────────────────────────────────
    int       osc1Table       = 0;
    float     osc1Position    = 0.0f;
    int       osc1Coarse      = 0;      // semitones
    float     osc1Fine        = 0.0f;   // cents
    float     osc1Level       = 0.8f;
    float     osc1Pan         = 0.0f;
    int       osc1UnisonVoices = 1;     // mapped from choice index: 0→1,1→2,2→3,3→4,4→6,5→8
    float     osc1Detune      = 0.0f;
    PhaseMode osc1PhaseMode   = PhaseMode::FREE;
    WTMode    osc1WTMode      = WTMode::SINGLE;

    // ── OSC 2 ──────────────────────────────────
    int       osc2Table       = 0;
    float     osc2Position    = 0.0f;
    int       osc2Coarse      = 0;
    float     osc2Fine        = 0.0f;
    float     osc2Level       = 0.8f;
    float     osc2Pan         = 0.0f;
    int       osc2UnisonVoices = 1;
    float     osc2Detune      = 0.0f;
    PhaseMode osc2PhaseMode   = PhaseMode::FREE;
    WTMode    osc2WTMode      = WTMode::SINGLE;

    // ── OSC stereo width (0=mono, 1=full spread) ─────
    float osc1Width = 1.0f;
    float osc2Width = 1.0f;

    // ── Pitch envelope (hardwired to both OSCs) ───────
    float pitchEnvAttack = 0.01f;
    float pitchEnvAmount = 0.0f;   // semitones, bipolar

    // ── OSC Mix ────────────────────────────────
    float oscMix     = 0.5f;
    int   oscMixMode = 0;
    float oscFMRatio = 1.0f;

    // ── Filter ─────────────────────────────────
    FilterType filterType     = FilterType::LP24;
    float      filterCutoff   = 5000.0f;
    float      filterRes      = 0.0f;
    float      filterDrive    = 0.0f;
    float      filterKeyTrack = 0.0f;
    float      filterVelTrack = 0.0f;
    int        filterRoute    = 0;  // 0=Both, 1=OSC1 only, 2=OSC2 only

    // ── Sub oscillator + noise ──────────────────
    float subLevel   = 0.0f;
    int   subOctave  = 0;     // 0 = -1 oct, 1 = -2 oct
    float noiseLevel = 0.0f;
    float noiseColor = 0.5f;  // 0=dark, 1=white

    // ── ENV 1 (→ filter cutoff) ────────────────
    float env1Attack   = 0.01f;
    float env1Decay    = 0.1f;
    float env1Sustain  = 0.7f;
    float env1Hold     = 0.0f;
    float env1Release  = 0.3f;
    CurveType env1Curve = CurveType::EXPONENTIAL;
    float env1Depth    = 0.5f;   // bipolar filter depth
    bool  env1Loop     = false;
    float env1VelSens  = 0.5f;

    // ── ENV 2 (→ VCA) ──────────────────────────
    float env2Attack   = 0.01f;
    float env2Decay    = 0.1f;
    float env2Sustain  = 0.7f;
    float env2Hold     = 0.0f;
    float env2Release  = 0.3f;
    CurveType env2Curve = CurveType::EXPONENTIAL;
    bool  env2Loop     = false;
    float env2VelSens  = 0.5f;

    // ── ENV 3 (free) ───────────────────────────
    float env3Attack   = 0.01f;
    float env3Decay    = 0.1f;
    float env3Sustain  = 0.7f;
    float env3Hold     = 0.0f;
    float env3Release  = 0.3f;
    CurveType env3Curve = CurveType::EXPONENTIAL;
    bool  env3Loop     = false;
    float env3VelSens  = 0.5f;

    // ── LFO 1 (→ WT position by default) ───────
    LFOShape lfo1Shape    = LFOShape::SINE;
    float    lfo1Rate     = 1.0f;
    bool     lfo1TempoSync = false;
    float    lfo1SyncDiv  = 0.5f;
    float    lfo1Phase    = 0.0f;
    int      lfo1Trigger  = 0;
    float    lfo1Fade     = 0.0f;
    float    lfo1Depth    = 0.5f;
    int      lfo1Target   = 0;

    // ── LFO 2 (→ filter cutoff by default) ─────
    LFOShape lfo2Shape    = LFOShape::SINE;
    float    lfo2Rate     = 1.0f;
    bool     lfo2TempoSync = false;
    float    lfo2SyncDiv  = 0.5f;
    float    lfo2Phase    = 0.0f;
    int      lfo2Trigger  = 0;
    float    lfo2Fade     = 0.0f;
    float    lfo2Depth    = 0.5f;
    int      lfo2Target   = 0;

    // ── Macros ─────────────────────────────────
    float macro[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    // ── Global voice settings ──────────────────
    float masterTune   = 0.0f;
    float masterVolume = 0.8f;
    double hostBPM     = 120.0;
    float modWheel     = 0.0f;   // CC1
    float aftertouch   = 0.0f;   // channel pressure
};

// Maps the choice-parameter index {0,1,2,3,4,5} to actual unison voice counts.
inline int unisonChoiceToCount(int choiceIndex)
{
    static const int map[6] = { 1, 2, 3, 4, 6, 8 };
    return (choiceIndex >= 0 && choiceIndex < 6) ? map[choiceIndex] : 1;
}

// ─────────────────────────────────────────────────────────────────────────────

struct Voice
{
    UnisonVoice       osc1, osc2;
    Filter            filterL, filterR;
    EnvelopeGenerator env1, env2, env3, pitchEnv;   // pitchEnv hardwired to both OSCs
    LFO               lfo1, lfo2;
    ModValues         modValues;

    int   midiNote  = -1;
    float velocity  = 0.0f;
    int   age       = 0;
    bool  active    = false;
    bool  releasing = false;

    juce::SmoothedValue<float> glide;

    // Pre-allocated render buffers
    std::vector<float> tmpL, tmpR, tmpL2, tmpR2;

    // ── Sub osc + noise state ──────────────────────────────────────────────
    float subPhase  = 0.0f;
    float noiseLP   = 0.0f;

    // ── Anti-click ─────────────────────────────────────────────────────────
    float antiClickGain = 0.0f;  // ramps 0→1 at note start; applied before filter
    float stealFade     = 1.0f;  // ramps 1→0 when steal queued; applied to output gain
    bool  hasPending    = false;  // true while waiting for steal fade to finish
    int   pendingNote   = -1;
    float pendingVel    = 0.0f;
    bool  pendingGlide  = false;

    // ── Analogue imperfection state ────────────────────────────────────────
    // Pitch drift: brownian random walk, ±1.5 cents max
    float pitchDrift    = 0.0f;
    float pitchDriftVel = 0.0f;
    // Wavetable position spectral drift (brownian)
    float wtDriftPos    = 0.0f;
    float wtDriftVel    = 0.0f;
    // Per-voice RNG (audio-thread safe, never shared)
    juce::Random rng;

    void prepare(double sampleRate, int blockSize);
    void reset();
};

enum class GlideMode { ALWAYS, LEGATO_ONLY };
enum class StealMode { OLDEST, LOWEST, HIGHEST, QUIETEST };

class VoiceManager
{
public:
    static constexpr int MAX_VOICES = 16;

    VoiceManager();

    void prepare(double sampleRate, int blockSize);
    void setMaxVoices(int n);
    void setMono(bool mono);
    void setGlideTime(float seconds);
    void setGlideMode(GlideMode mode);
    void setStealMode(StealMode mode);
    void setModMatrix(const ModMatrix* matrix);

    void setWavetableBank(WavetableBank* bank) { wavetableBank = bank; }

    // Apply snapshot to all active voices — call once per block before process()
    void updateFromParams(const SynthParamSnapshot& p);

    void noteOn(int midiNote, float velocity);
    void noteOff(int midiNote);
    void allNotesOff();
    void setPitchBend(float semitones);
    void setSustain(bool on);

    // Render all active voices into the stereo buffer (additive)
    void process(juce::AudioBuffer<float>& buffer);

private:
    Voice   voices[MAX_VOICES];
    int     maxVoices   = 8;
    bool    monoMode    = false;
    float   glideTime   = 0.0f;
    float   pitchBend   = 0.0f;
    GlideMode glideMode = GlideMode::LEGATO_ONLY;
    StealMode stealMode = StealMode::OLDEST;
    double  sampleRate  = 44100.0;
    int     blockSize   = 512;
    int     voiceAge    = 0;

    bool    sustainPedal = false;
    juce::Array<int> sustainedNotes;

    const ModMatrix*   modMatrix     = nullptr;
    WavetableBank*     wavetableBank = nullptr;
    SynthParamSnapshot lastParams;

    Voice* findFreeVoice();
    Voice* stealVoice();
    void   applyParamsToVoice(Voice& v, const SynthParamSnapshot& p) const;
    void   activateVoice(Voice& v, int note, float vel, bool shouldGlide);

    float  antiClickRate = 0.0f;   // per-sample: ramps antiClickGain 0→1 over 5ms
    float  stealFadeRate = 0.0f;   // per-sample: ramps stealFade 1→0 over 3ms

    static float midiNoteToHz(int note, float pitchBendSemis = 0.0f,
                               float masterTuneCents = 0.0f);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoiceManager)
};
