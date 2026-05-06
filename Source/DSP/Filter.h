#pragma once
#include <JuceHeader.h>

enum class FilterType { LP12, LP24, HP12, HP24, BP, NOTCH, COMB, FORMANT };

// LP12/HP12/BP/Notch/Comb/Formant: SVF (Simper/Zavalishin TPT)
// LP24:  Moog-style ladder (Huovilainen model) — 4 one-pole stages + tanh feedback
// HP24:  Two cascaded SVF HP12 stages
class Filter
{
public:
    Filter();

    void prepare(double sampleRate, int blockSize);
    void reset();

    void setType(FilterType type);
    void setCutoff(float hz);
    void setResonance(float r);
    void setDrive(float d);
    void setKeyTracking(float kt);
    void setVelTracking(float vt);

    float processSample(float input);
    void  processBlock(float* data, int numSamples);

private:
    // SVF state
    struct SVFState { float ic1eq = 0.0f, ic2eq = 0.0f; };
    SVFState stage1, stage2;

    // Moog ladder state (4 one-pole stages)
    struct LadderState { float y[4] = {0,0,0,0}; };
    LadderState ladder;

    // Comb filter
    static constexpr int COMB_MAX = 48000;
    std::vector<float> combBuffer;
    int combWritePos = 0;

    // Formant filter
    struct BPState { float ic1eq = 0.0f, ic2eq = 0.0f; };
    BPState formantBP[3];

    FilterType type     = FilterType::LP24;
    double sampleRate   = 44100.0;
    float  cutoff       = 5000.0f;
    float  resonance    = 0.0f;
    float  drive        = 0.0f;

    // SVF coefficients
    float g  = 0.0f, k  = 0.0f;
    float a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;

    // Ladder coefficient (one-pole LP gain)
    float g_lp = 0.0f;

    void  updateCoefficients();
    float applySVF(float input, SVFState& s, bool returnLP) const;
    float applyLadder(float input);
    float applyComb(float input);
    float applyFormant(float input);
    float softClip(float x) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Filter)
};
