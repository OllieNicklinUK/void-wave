#include "LFO.h"

LFO::LFO() = default;

void LFO::prepare(double sr)
{
    sampleRate = sr;
    updatePhaseInc();
}

void LFO::reset()
{
    phase      = phaseOffset;
    fadeSample = 0.0f;
    smValue    = 0.0f;
    smTarget   = 0.0f;
}

void LFO::setShape(LFOShape s)      { shape = s; }
void LFO::setDepth(float d)         { depth = d; }
void LFO::setTriggerMode(int m)     { trigMode = m; }
void LFO::setTempoSync(bool e)      { tempoSync = e; updatePhaseInc(); }
void LFO::setSyncDivision(float d)  { syncDiv = d; updatePhaseInc(); }
void LFO::setHostBPM(double bpm)    { hostBPM = bpm; updatePhaseInc(); }

void LFO::setPhaseOffset(float degrees)
{
    phaseOffset = degrees / 360.0f;
}

void LFO::setFadeIn(float seconds)
{
    fadeIn      = seconds;
    fadeSamples = seconds * static_cast<float>(sampleRate);
}

void LFO::setRate(float hz)
{
    rate = hz;
    if (!tempoSync) updatePhaseInc();
}

void LFO::updatePhaseInc()
{
    float rateHz = rate;
    if (tempoSync)
    {
        double beatsPerSec = hostBPM / 60.0;
        // syncDiv is choice index: 0="1/1", 1="1/2", 2="1/4", 3="1/8", 4="1/16", 5="1/32"
        float divMap[6] = { 4.0f, 2.0f, 1.0f, 0.5f, 0.25f, 0.125f };
        int idx = juce::jlimit(0, 5, static_cast<int>(syncDiv));
        float beatsPerCycle = divMap[idx];
        rateHz = static_cast<float>(beatsPerSec / beatsPerCycle);
    }
    phaseInc = rateHz / static_cast<float>(sampleRate);
}

void LFO::noteOn()
{
    if (trigMode == 1 || trigMode == 2)
        reset();
    fadeSample = 0.0f;
    running    = (trigMode != 2);   // ONE_SHOT will self-stop after one cycle
}

float LFO::generateShape() const
{
    float p = phase;

    switch (shape)
    {
        case LFOShape::SINE:
            return std::sin(p * juce::MathConstants<float>::twoPi);

        case LFOShape::TRI:
            return (p < 0.5f) ? (4.0f * p - 1.0f) : (3.0f - 4.0f * p);

        case LFOShape::SAW:
            return 2.0f * p - 1.0f;

        case LFOShape::RAMP:
            return 1.0f - 2.0f * p;

        case LFOShape::SQUARE:
            return (p < 0.5f) ? 1.0f : -1.0f;

        case LFOShape::S_AND_H:
        case LFOShape::SMOOTH_RAND:
            return 0.0f; // handled in getNextSample
    }
    return 0.0f;
}

float LFO::getNextSample()
{
    if (!running) return 0.0f;

    float output = 0.0f;

    if (shape == LFOShape::S_AND_H)
    {
        // Sample new value on each cycle start
        output = shValue;
    }
    else if (shape == LFOShape::SMOOTH_RAND)
    {
        smValue += (smTarget - smValue) * 0.01f;
        output = smValue;
    }
    else
    {
        output = generateShape();
    }

    bool wrapped = false;
    phase += phaseInc;
    if (phase >= 1.0f)
    {
        phase -= 1.0f;
        wrapped = true;

        if (shape == LFOShape::S_AND_H)
            shValue = juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
        else if (shape == LFOShape::SMOOTH_RAND)
            smTarget = juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;

        if (trigMode == 2)  // ONE_SHOT stops after one cycle
            running = false;
    }
    juce::ignoreUnused(wrapped);

    // Fade-in envelope
    float fadeGain = 1.0f;
    if (fadeSamples > 0.0f)
    {
        fadeGain    = juce::jlimit(0.0f, 1.0f, fadeSample / fadeSamples);
        fadeSample += 1.0f;
    }

    return output * fadeGain;
}

float LFO::processBlock(int numSamples)
{
    // Advance n-1 samples silently, then return sample n.
    // This is O(n) but correct — phaseInc is per-sample, not per-block.
    for (int i = 0; i < numSamples - 1; ++i)
        getNextSample();
    return getNextSample();
}
