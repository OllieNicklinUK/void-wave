#pragma once
#include <JuceHeader.h>
#include "Peverb/LateReverb.h"
#include "Peverb/EarlyReflections.h"

// Peverb-based algorithmic reverb.
// Time-varying nested all-pass algorithm with early reflections.
// Public interface unchanged from the old Freeverb implementation.
class VWReverb
{
public:
    VWReverb()  = default;
    ~VWReverb() = default;

    void prepare(double sampleRate, int blockSize);
    void reset();

    void setSize(float size);       // 0–1 → feedback gain 0.15–0.88
    void setDamping(float damp);    // 0–1 → post-LP cutoff 8000→500 Hz
    void setPredelay(float ms);     // 0–200 ms
    void setWidth(float width);     // 0–1 → stereo width (unused — Peverb is inherently stereo)
    void setMix(float mix);         // 0–1 wet/dry
    void setEnabled(bool on);

    void process(juce::AudioBuffer<float>& buffer);

private:
    std::unique_ptr<LateReverb>       late;
    std::unique_ptr<EarlyReflections> early;

    double sampleRate  = 44100.0;
    float  size        = 0.5f;
    float  damping     = 0.5f;
    float  width       = 1.0f;
    float  mix         = 0.3f;
    bool   enabled     = true;
    float  predelayMs  = 0.0f;

    // Running time counter (seconds) — feeds Peverb's time-varying modulation
    float  t  = 0.0f;
    double dt = 1.0 / 44100.0;

    // One-pole LP for post-damping and pre-delay circular buffer
    float  dampStateL = 0.0f, dampStateR = 0.0f;

    static constexpr int MAX_PREDELAY = 9600;
    std::vector<float> predelayBufL, predelayBufR;
    int predelayPos     = 0;
    int predelaySamples = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VWReverb)
};
