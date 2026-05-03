#pragma once
#include <JuceHeader.h>

enum class ModFXType { CHORUS, ENSEMBLE, FLANGER, PHASER };

class Chorus
{
public:
    static constexpr int MAX_DELAY_SAMPLES = 8192;

    Chorus();
    void prepare(double sampleRate, int blockSize);
    void reset();

    void setType(ModFXType type);
    void setRate(float hz);     // 0.01–20
    void setDepth(float depth); // 0.0–1.0
    void setFeedback(float fb); // 0.0–1.0 (flanger/phaser)
    void setMix(float mix);     // 0.0–1.0
    void setEnabled(bool on);

    void process(juce::AudioBuffer<float>& buffer);

private:
    ModFXType type    = ModFXType::CHORUS;
    float     rate    = 0.5f;
    float     depth   = 0.5f;
    float     feedback = 0.0f;
    float     mix     = 0.5f;
    bool      enabled = true;
    double    sampleRate = 44100.0;

    // Delay line for chorus/flanger
    std::vector<float> delayL, delayR;
    int writePos = 0;

    // LFO phase for modulation
    float lfoPhase = 0.0f;
    float lfoInc   = 0.0f;

    // Phaser all-pass state (4 stages per channel)
    float apStateL[4] = {}, apStateR[4] = {};

    void updateLFORate();
    float readDelay(const std::vector<float>& buf, float delaySamples) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Chorus)
};
