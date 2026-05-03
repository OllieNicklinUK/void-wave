#pragma once
#include <JuceHeader.h>

enum class LFOShape { SINE, TRI, SAW, RAMP, SQUARE, S_AND_H, SMOOTH_RAND };

class LFO
{
public:
    LFO();

    void prepare(double sampleRate);
    void reset();

    void setShape(LFOShape shape);
    void setRate(float hz);             // 0.01–20 Hz (free mode)
    void setTempoSync(bool enabled);
    void setSyncDivision(float div);    // beats (0.03125 = 1/32, 8.0 = 8 bars)
    void setPhaseOffset(float degrees); // 0–360
    void setTriggerMode(int mode);      // 0=FREE, 1=RETRIG, 2=ONE_SHOT
    void setFadeIn(float seconds);      // 0–10 s
    void setDepth(float depth);         // -1.0–1.0 (applied externally via mod matrix)

    // Call once per buffer from PluginProcessor when host BPM is available
    void setHostBPM(double bpm);

    void noteOn();

    float getNextSample();

    // Advance by numSamples and return the value at the last sample.
    // Use this from VoiceManager::process() instead of calling getNextSample() once,
    // because phaseInc is sized for per-sample rate.
    float processBlock(int numSamples);

private:
    double sampleRate  = 44100.0;
    float  rate        = 1.0f;
    float  phase       = 0.0f;      // 0.0–1.0
    float  phaseInc    = 0.0f;
    float  phaseOffset = 0.0f;      // normalised (0–1)
    float  depth       = 0.5f;
    float  fadeIn      = 0.0f;
    float  fadeSample  = 0.0f;
    float  fadeSamples = 0.0f;

    LFOShape shape       = LFOShape::SINE;
    bool     tempoSync   = false;
    float    syncDiv     = 0.5f;
    int      trigMode    = 0;
    bool     running     = true;
    double   hostBPM     = 120.0;

    float shValue = 0.0f;   // S&H held value
    float smValue = 0.0f;   // smooth random current
    float smTarget = 0.0f;  // smooth random target

    void updatePhaseInc();
    float generateShape() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LFO)
};
