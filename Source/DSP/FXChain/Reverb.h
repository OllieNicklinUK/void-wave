#pragma once
#include <JuceHeader.h>

// Freeverb-style algorithmic reverb: 8 parallel comb filters + 4 series all-pass.
class VWReverb
{
public:
    static constexpr int NUM_COMBS   = 8;
    static constexpr int NUM_ALLPASS = 4;

    VWReverb();
    void prepare(double sampleRate, int blockSize);
    void reset();

    void setSize(float size);       // 0.0–1.0 → room size (scales comb delay times)
    void setDamping(float damp);    // 0.0–1.0 → LP damping inside combs
    void setPredelay(float ms);     // 0–200 ms
    void setWidth(float width);     // 0.0–1.0 → stereo width
    void setMix(float mix);         // 0.0–1.0
    void setEnabled(bool on);

    void process(juce::AudioBuffer<float>& buffer);

private:
    // Freeverb tuning constants (samples at 44100 Hz, scaled at runtime)
    static const int COMB_TUNING_L[NUM_COMBS];
    static const int COMB_TUNING_R[NUM_COMBS];
    static const int AP_TUNING_L[NUM_ALLPASS];
    static const int AP_TUNING_R[NUM_ALLPASS];

    struct CombFilter
    {
        std::vector<float> buf;
        int   writePos  = 0;
        float feedback  = 0.84f;
        float dampState = 0.0f;
        float dampCoeff = 0.2f;

        void prepare(int size);
        void reset();
        float process(float input);
    };

    struct AllPassFilter
    {
        std::vector<float> buf;
        int   writePos = 0;
        float feedback = 0.5f;

        void prepare(int size);
        void reset();
        float process(float input);
    };

    CombFilter    combL[NUM_COMBS],   combR[NUM_COMBS];
    AllPassFilter allPassL[NUM_ALLPASS], allPassR[NUM_ALLPASS];

    // Pre-delay
    static constexpr int MAX_PREDELAY = 9600; // 200ms @ 48kHz
    std::vector<float> predelayBuf;
    int predelayPos     = 0;
    int predelaySamples = 0;

    float size    = 0.5f;
    float damping = 0.5f;
    float width   = 1.0f;
    float mix     = 0.3f;
    bool  enabled = true;
    double sampleRate = 44100.0;

    void updateParams();
    int scaleToSampleRate(int baseAt44100) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VWReverb)
};
