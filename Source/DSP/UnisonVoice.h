#pragma once
#include <JuceHeader.h>
#include "WavetableOscillator.h"

class UnisonVoice
{
public:
    static constexpr int MAX_VOICES = 8;

    UnisonVoice();

    void prepare(double sampleRate, int blockSize);
    void reset();

    void setFrequency(float hz);
    void setNumVoices(int n);
    void setDetune(float amount);
    void setWidth(float w);    // 0=mono centre, 1=full stereo spread
    void setTableIndex(int index);
    void setTableData(const float* data);
    void setPosition(float pos);
    void setLevel(float level);
    void setPhaseMode(PhaseMode mode);
    void setWTMode(WTMode mode);
    void triggerNote();

    void process(float* outL, float* outR, int numSamples);

private:
    WavetableOscillator voices[MAX_VOICES];
    int    numVoices  = 1;
    float  baseFreq   = 440.0f;
    float  detune     = 0.0f;
    float  width      = 1.0f;   // stereo spread 0=mono, 1=full
    double sampleRate = 44100.0;

    // Per-voice detuned frequencies (computed in applyDetuneAndPan)
    float  voiceFreqs[MAX_VOICES] = {};

    // Per-voice micro-modulation: independent slow LFOs give organic movement.
    // Pitch (0.3–2.5 cents), level (0.5–2%), pan (1–3%) — too small to hear as
    // vibrato but enough that the beating pattern never repeats.
    struct MicroMod
    {
        float phase      = 0.0f;
        float phaseInc   = 0.0f;   // per sample
        float pitchCents = 1.0f;   // max depth in cents
        float levelAmt   = 0.01f;  // max level fraction
    };
    MicroMod microMod[MAX_VOICES];

    void applyDetuneAndPan();
    void initMicroMod();           // randomise on prepare/triggerNote

    juce::Random rng;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UnisonVoice)
};
