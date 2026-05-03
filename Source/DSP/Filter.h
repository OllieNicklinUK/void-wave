#pragma once
#include <JuceHeader.h>

enum class FilterType { LP12, LP24, HP12, HP24, BP, NOTCH, COMB, FORMANT };

// State Variable Filter (Simper/Zavalishin TPT) covering LP/HP/BP/Notch.
// LP24 and HP24 are two cascaded SVF stages.
// Comb and Formant are separate implementations in the same class.
class Filter
{
public:
    Filter();

    void prepare(double sampleRate, int blockSize);
    void reset();

    void setType(FilterType type);
    void setCutoff(float hz);       // 20–20000
    void setResonance(float r);     // 0.0–1.0 (self-oscillates at 1.0)
    void setDrive(float d);         // 0.0–1.0 → pre-filter soft clip
    void setKeyTracking(float kt);  // 0.0–2.0; applied externally per note
    void setVelTracking(float vt);  // 0.0–1.0

    float processSample(float input);
    void  processBlock(float* data, int numSamples);

private:
    // SVF state (two stages for 24dB modes)
    struct SVFState { float ic1eq = 0.0f, ic2eq = 0.0f; };
    SVFState stage1, stage2;

    // Comb filter state
    static constexpr int COMB_MAX = 48000;
    std::vector<float> combBuffer;
    int combWritePos = 0;

    // Formant filter (3× bandpass, vowel A/E/I/O/U)
    struct BPState { float ic1eq = 0.0f, ic2eq = 0.0f; };
    BPState formantBP[3];

    FilterType type       = FilterType::LP24;
    double sampleRate     = 44100.0;
    float  cutoff         = 5000.0f;
    float  resonance      = 0.0f;
    float  drive          = 0.0f;

    float g  = 0.0f;
    float k  = 0.0f;
    float a1 = 0.0f;
    float a2 = 0.0f;
    float a3 = 0.0f;

    void  updateCoefficients();
    float applySVF(float input, SVFState& s, bool returnLP) const;
    float applyComb(float input);
    float applyFormant(float input);
    float softClip(float x) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Filter)
};
