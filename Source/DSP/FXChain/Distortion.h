#pragma once
#include <JuceHeader.h>

enum class DistType { SOFT_CLIP, HARD_CLIP, TUBE_SAT, FOLDBACK, BITCRUSHER };

class Distortion
{
public:
    Distortion();
    void prepare(double sampleRate, int blockSize);
    void reset();

    void setType(DistType type);
    void setDrive(float drive);   // 0.0–1.0
    void setTone(float hz);       // 20–20000 (post HP filter)
    void setMix(float mix);       // 0.0–1.0
    void setEnabled(bool on);

    void process(juce::AudioBuffer<float>& buffer);

private:
    DistType type    = DistType::SOFT_CLIP;
    float    drive   = 0.0f;
    float    tone    = 8000.0f;
    float    mix     = 1.0f;
    bool     enabled = true;

    // Simple one-pole HP for tone control
    float hpStateL = 0.0f, hpStateR = 0.0f;
    float hpCoeff  = 0.0f;
    double sampleRate = 44100.0;

    void updateTone();
    float processSample(float x) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Distortion)
};
