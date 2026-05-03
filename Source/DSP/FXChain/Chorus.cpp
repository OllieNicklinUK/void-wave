#include "Chorus.h"

Chorus::Chorus() = default;

void Chorus::prepare(double sr, int /*blockSize*/)
{
    sampleRate = sr;
    delayL.assign(MAX_DELAY_SAMPLES, 0.0f);
    delayR.assign(MAX_DELAY_SAMPLES, 0.0f);
    reset();
    updateLFORate();
}

void Chorus::reset()
{
    writePos  = 0;
    lfoPhase  = 0.0f;
    std::fill(delayL.begin(), delayL.end(), 0.0f);
    std::fill(delayR.begin(), delayR.end(), 0.0f);
    for (auto& s : apStateL) s = 0.0f;
    for (auto& s : apStateR) s = 0.0f;
}

void Chorus::setType(ModFXType t)   { type = t; }
void Chorus::setDepth(float d)      { depth = d; }
void Chorus::setFeedback(float fb)  { feedback = juce::jlimit(0.0f, 0.98f, fb); }
void Chorus::setMix(float m)        { mix = m; }
void Chorus::setEnabled(bool on)    { enabled = on; }

void Chorus::setRate(float hz)
{
    rate = hz;
    updateLFORate();
}

void Chorus::updateLFORate()
{
    lfoInc = rate / static_cast<float>(sampleRate);
}

float Chorus::readDelay(const std::vector<float>& buf, float delaySamples) const
{
    float pos = static_cast<float>(writePos) - delaySamples;
    if (pos < 0.0f) pos += static_cast<float>(MAX_DELAY_SAMPLES);
    int   ia  = static_cast<int>(pos) % MAX_DELAY_SAMPLES;
    int   ib  = (ia + 1) % MAX_DELAY_SAMPLES;
    float frac = pos - static_cast<float>(static_cast<int>(pos));
    return buf[static_cast<size_t>(ia)] + frac * (buf[static_cast<size_t>(ib)] - buf[static_cast<size_t>(ia)]);
}

void Chorus::process(juce::AudioBuffer<float>& buffer)
{
    if (!enabled) return;

    auto* L = buffer.getWritePointer(0);
    auto* R = buffer.getWritePointer(1);
    int   n = buffer.getNumSamples();

    for (int i = 0; i < n; ++i)
    {
        float lfoVal = std::sin(lfoPhase * juce::MathConstants<float>::twoPi);
        lfoPhase += lfoInc;
        if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;

        float dryL = L[i], dryR = R[i];

        if (type == ModFXType::PHASER)
        {
            float wetL = dryL, wetR = dryR;
            float coeff = lfoVal * depth * 0.99f;
            for (int stage = 0; stage < 4; ++stage)
            {
                float outL = coeff * (wetL - apStateL[stage]) + apStateL[stage];
                apStateL[stage] = wetL;
                wetL = outL;

                float outR = coeff * (wetR - apStateR[stage]) + apStateR[stage];
                apStateR[stage] = wetR;
                wetR = outR;
            }
            L[i] = dryL + mix * (wetL - dryL);
            R[i] = dryR + mix * (wetR - dryR);
        }
        else
        {
            // Chorus / Flanger / Ensemble use a modulated delay line
            float baseDelay = (type == ModFXType::FLANGER) ? 2.0f : 20.0f;  // ms
            float modDepth  = depth * baseDelay;
            float delaySampL = (baseDelay + lfoVal * modDepth)
                              * static_cast<float>(sampleRate) * 0.001f;
            float delaySampR = (baseDelay - lfoVal * modDepth)   // opposite phase → stereo
                              * static_cast<float>(sampleRate) * 0.001f;

            delaySampL = juce::jlimit(1.0f, static_cast<float>(MAX_DELAY_SAMPLES - 2), delaySampL);
            delaySampR = juce::jlimit(1.0f, static_cast<float>(MAX_DELAY_SAMPLES - 2), delaySampR);

            float wetL = readDelay(delayL, delaySampL);
            float wetR = readDelay(delayR, delaySampR);

            delayL[static_cast<size_t>(writePos)] = dryL + wetL * feedback;
            delayR[static_cast<size_t>(writePos)] = dryR + wetR * feedback;
            writePos = (writePos + 1) % MAX_DELAY_SAMPLES;

            L[i] = dryL + mix * (wetL - dryL);
            R[i] = dryR + mix * (wetR - dryR);
        }
    }
}
