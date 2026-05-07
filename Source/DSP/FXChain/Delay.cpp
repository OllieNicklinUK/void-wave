#include "Delay.h"

Delay::Delay() = default;

void Delay::prepare(double sr, int /*blockSize*/)
{
    sampleRate = sr;
    bufL.assign(MAX_DELAY_SAMPLES, 0.0f);
    bufR.assign(MAX_DELAY_SAMPLES, 0.0f);
    reset();
}

void Delay::reset()
{
    writePos   = 0;
    dampStateL = dampStateR = 0.0f;
    std::fill(bufL.begin(), bufL.end(), 0.0f);
    std::fill(bufR.begin(), bufR.end(), 0.0f);
}

void Delay::setType(DelayType t)   { type = t; }
void Delay::setFeedback(float fb)  { feedback = juce::jlimit(0.0f, 0.98f, fb); }
void Delay::setDamping(float d)    { damping  = d; }
void Delay::setMix(float m)        { mix = m; }
void Delay::setEnabled(bool on)    { enabled = on; }

void Delay::setTimeMs(float ms)
{
    timeSamples = juce::jlimit(1.0f, static_cast<float>(MAX_DELAY_SAMPLES - 1),
                               ms * static_cast<float>(sampleRate) * 0.001f);
}

void Delay::setTempoSync(bool /*enabled*/, double bpm, float division)
{
    double beatsPerSec = bpm / 60.0;
    
    // division comes in as 0..2000 from the Time knob.
    // We map it to 6 discrete steps: 1/32, 1/16, 1/8, 1/4D, 1/4, 1/2
    float norm = juce::jlimit(0.0f, 1.0f, division / 2000.0f);
    int step = static_cast<int>(norm * 5.99f);
    float syncDivs[6] = { 0.125f, 0.25f, 0.5f, 0.75f, 1.0f, 2.0f }; // fraction of a beat
    float actualDiv = syncDivs[step];

    float periodMs = static_cast<float>(actualDiv / beatsPerSec) * 1000.0f;
    setTimeMs(periodMs);
}

float Delay::readAt(const std::vector<float>& buf, float delaySamples) const
{
    float pos = static_cast<float>(writePos) - delaySamples;
    if (pos < 0.0f) pos += static_cast<float>(MAX_DELAY_SAMPLES);
    int   ia   = static_cast<int>(pos) % MAX_DELAY_SAMPLES;
    int   ib   = (ia + 1) % MAX_DELAY_SAMPLES;
    float frac = pos - static_cast<float>(static_cast<int>(pos));
    return buf[static_cast<size_t>(ia)] + frac * (buf[static_cast<size_t>(ib)] - buf[static_cast<size_t>(ia)]);
}

void Delay::process(juce::AudioBuffer<float>& buffer)
{
    if (!enabled) return;

    auto* L = buffer.getWritePointer(0);
    auto* R = buffer.getWritePointer(1);
    int   n = buffer.getNumSamples();

    float dampCoeff = 1.0f - damping * 0.9f;

    for (int i = 0; i < n; ++i)
    {
        float dryL = L[i], dryR = R[i];
        float wetL, wetR;

        switch (type)
        {
            case DelayType::PING_PONG:
                wetL = readAt(bufR, timeSamples);  // R feeds L
                wetR = readAt(bufL, timeSamples);  // L feeds R
                dampStateL = dampStateL + dampCoeff * (wetL - dampStateL);
                dampStateR = dampStateR + dampCoeff * (wetR - dampStateR);
                bufL[static_cast<size_t>(writePos)] = dryL + dampStateR * feedback;
                bufR[static_cast<size_t>(writePos)] = dryR + dampStateL * feedback;
                break;

            case DelayType::STEREO:
                wetL = readAt(bufL, timeSamples);
                wetR = readAt(bufR, timeSamples * 1.02f);  // slight offset for width
                dampStateL = dampStateL + dampCoeff * (wetL - dampStateL);
                dampStateR = dampStateR + dampCoeff * (wetR - dampStateR);
                bufL[static_cast<size_t>(writePos)] = dryL + dampStateL * feedback;
                bufR[static_cast<size_t>(writePos)] = dryR + dampStateR * feedback;
                break;

            default: // MONO
                wetL = wetR = readAt(bufL, timeSamples);
                dampStateL  = dampStateL + dampCoeff * (wetL - dampStateL);
                bufL[static_cast<size_t>(writePos)] = (dryL + dryR) * 0.5f + dampStateL * feedback;
                break;
        }

        writePos = (writePos + 1) % MAX_DELAY_SAMPLES;

        L[i] = dryL + mix * (wetL - dryL);
        R[i] = dryR + mix * (wetR - dryR);
    }
}
