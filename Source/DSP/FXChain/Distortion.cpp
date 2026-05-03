#include "Distortion.h"

Distortion::Distortion() = default;

void Distortion::prepare(double sr, int /*blockSize*/)
{
    sampleRate = sr;
    updateTone();
}

void Distortion::reset()
{
    hpStateL = hpStateR = 0.0f;
}

void Distortion::setType(DistType t)   { type  = t; }
void Distortion::setDrive(float d)     { drive = d; }
void Distortion::setMix(float m)       { mix   = m; }
void Distortion::setEnabled(bool on)   { enabled = on; }

void Distortion::setTone(float hz)
{
    tone = hz;
    updateTone();
}

void Distortion::updateTone()
{
    // One-pole HP: coeff = e^(-2π * fc / sr)
    hpCoeff = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi
                               * tone / static_cast<float>(sampleRate));
}

float Distortion::processSample(float x) const
{
    float driveAmt = 1.0f + drive * 60.0f;
    switch (type)
    {
        case DistType::SOFT_CLIP:
            return (juce::MathConstants<float>::pi + driveAmt) * x
                 / (juce::MathConstants<float>::pi + driveAmt * std::abs(x));

        case DistType::HARD_CLIP:
            return juce::jlimit(-1.0f / driveAmt, 1.0f / driveAmt, x) * driveAmt;

        case DistType::TUBE_SAT:
            return std::tanh(driveAmt * x) / std::tanh(driveAmt);

        case DistType::FOLDBACK:
        {
            float folded = x * driveAmt;
            while (std::abs(folded) > 1.0f)
                folded = std::abs(folded) > 1.0f ? 2.0f * std::copysign(1.0f, folded) - folded : folded;
            return folded;
        }

        case DistType::BITCRUSHER:
        {
            int bits = juce::jlimit(2, 16, static_cast<int>(16.0f - drive * 14.0f));
            float levels = std::pow(2.0f, static_cast<float>(bits));
            return std::round(x * levels) / levels;
        }
    }
    return x;
}

void Distortion::process(juce::AudioBuffer<float>& buffer)
{
    if (!enabled) return;

    auto* L = buffer.getWritePointer(0);
    auto* R = buffer.getWritePointer(1);
    int n   = buffer.getNumSamples();

    for (int i = 0; i < n; ++i)
    {
        float dryL = L[i], dryR = R[i];

        float wetL = processSample(L[i]);
        float wetR = processSample(R[i]);

        // Post-distortion HP tone control
        hpStateL += hpCoeff * (wetL - hpStateL);
        hpStateR += hpCoeff * (wetR - hpStateR);
        wetL -= hpStateL;
        wetR -= hpStateR;

        L[i] = dryL + mix * (wetL - dryL);
        R[i] = dryR + mix * (wetR - dryR);
    }
}
