#pragma once
#include <JuceHeader.h>

enum class DelayType { MONO, PING_PONG, STEREO };

class Delay
{
public:
    static constexpr int MAX_DELAY_SAMPLES = 192000; // 4 s @ 48kHz

    Delay();
    void prepare(double sampleRate, int blockSize);
    void reset();

    void setType(DelayType type);
    void setTimeMs(float ms);       // 1–2000 ms
    void setFeedback(float fb);     // 0.0–1.0
    void setDamping(float damp);    // 0.0–1.0 (simple LP on feedback path)
    void setMix(float mix);         // 0.0–1.0
    void setEnabled(bool on);

    // Tempo sync: call from PluginProcessor when BPM is known
    void setTempoSync(bool enabled, double bpm, float division);

    void process(juce::AudioBuffer<float>& buffer);

private:
    DelayType type    = DelayType::PING_PONG;
    float     timeSamples = 0.0f;
    float     feedback    = 0.4f;
    float     damping     = 0.0f;
    float     mix         = 0.3f;
    bool      enabled     = true;
    double    sampleRate  = 44100.0;

    std::vector<float> bufL, bufR;
    int writePos = 0;

    float dampStateL = 0.0f, dampStateR = 0.0f;

    float readAt(const std::vector<float>& buf, float delaySamples) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Delay)
};
