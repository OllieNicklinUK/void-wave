#include "Filter.h"

Filter::Filter()
{
    combBuffer.resize(COMB_MAX, 0.0f);
}

void Filter::prepare(double sr, int /*blockSize*/)
{
    sampleRate = sr;
    combBuffer.assign(static_cast<size_t>(sr), 0.0f);
    reset();
    updateCoefficients();
}

void Filter::reset()
{
    stage1 = stage2 = {};
    ladder = {};
    formantBP[0] = formantBP[1] = formantBP[2] = {};
    combWritePos = 0;
    std::fill(combBuffer.begin(), combBuffer.end(), 0.0f);
}

void Filter::setType(FilterType t)       { type = t; }
void Filter::setResonance(float r)       { resonance = r; updateCoefficients(); }
void Filter::setDrive(float d)           { drive = d; }
void Filter::setKeyTracking(float kt)    { juce::ignoreUnused(kt); }
void Filter::setVelTracking(float vt)    { juce::ignoreUnused(vt); }

void Filter::setCutoff(float hz)
{
    cutoff = juce::jlimit(20.0f, 20000.0f, hz);
    updateCoefficients();
}

void Filter::updateCoefficients()
{
    const float pi = juce::MathConstants<float>::pi;
    g  = std::tan(pi * cutoff / static_cast<float>(sampleRate));

    // SVF resonance: exponential curve spreads musical range across the knob
    {
        float r = juce::jlimit(0.0f, 0.9999f, resonance);
        k = 2.0f * std::pow(1.0f - r, 1.5f);
    }
    a1 = 1.0f / (1.0f + g * (g + k));
    a2 = g * a1;
    a3 = g * a2;

    // Ladder: bilinear one-pole LP coefficient
    g_lp = g / (1.0f + g);
}

float Filter::softClip(float x) const
{
    float driveAmt = 1.0f + drive * 60.0f;
    return (juce::MathConstants<float>::pi + driveAmt) * x
         / (juce::MathConstants<float>::pi + driveAmt * std::abs(x));
}

float Filter::applySVF(float input, SVFState& s, bool returnLP) const
{
    float v3 = input - s.ic2eq;
    float v1 = a1 * s.ic1eq + a2 * v3;
    float v2 = s.ic2eq + a2 * s.ic1eq + a3 * v3;
    s.ic1eq  = 2.0f * v1 - s.ic1eq;
    s.ic2eq  = 2.0f * v2 - s.ic2eq;

    // ── Non-linear resonance: saturate feedback states at high resonance ──────
    // Real analogue filters compress and colour as res increases — the
    // self-oscillation is slightly impure, not a clean sine.
    if (resonance > 0.45f)
    {
        float satAmt = (resonance - 0.45f) * 0.55f;  // 0 → 0.3 as res goes 0.45→1
        s.ic1eq = s.ic1eq / (1.0f + satAmt * std::abs(s.ic1eq) * 0.35f);
        s.ic2eq = s.ic2eq / (1.0f + satAmt * std::abs(s.ic2eq) * 0.18f);
    }

    if (returnLP) return v2;
    return input - k * v1 - v2;
}

float Filter::applyLadder(float input)
{
    float r = juce::jlimit(0.0f, 0.9999f, resonance);
    float res_exp = 1.0f - std::pow(1.0f - r, 1.5f);
    float res4 = 3.9f * res_exp;

    float x = std::tanh((input - res4 * ladder.y[3]) * 0.9f);

    ladder.y[0] += g_lp * (std::tanh(x)              - ladder.y[0]);
    ladder.y[1] += g_lp * (std::tanh(ladder.y[0])    - ladder.y[1]);
    ladder.y[2] += g_lp * (std::tanh(ladder.y[1])    - ladder.y[2]);
    ladder.y[3] += g_lp * (std::tanh(ladder.y[2])    - ladder.y[3]);

    return ladder.y[3] * (1.0f + res_exp * 0.5f);
}

float Filter::applyComb(float input)
{
    int delaySamples = static_cast<int>(sampleRate / cutoff);
    delaySamples = juce::jlimit(1, static_cast<int>(combBuffer.size()) - 1, delaySamples);
    int readPos = (combWritePos - delaySamples + static_cast<int>(combBuffer.size()))
                  % static_cast<int>(combBuffer.size());
    float delayed = combBuffer[static_cast<size_t>(readPos)];
    float out = input + resonance * delayed;
    combBuffer[static_cast<size_t>(combWritePos)] = out;
    combWritePos = (combWritePos + 1) % static_cast<int>(combBuffer.size());
    return out;
}

// Vowel formant frequencies (A, mid between A and E for simplicity)
static const float FORMANT_FREQS[3] = { 800.0f, 1200.0f, 2600.0f };

float Filter::applyFormant(float input)
{
    float out = 0.0f;
    for (int b = 0; b < 3; ++b)
    {
        float fc = FORMANT_FREQS[b];
        float gf = std::tan(juce::MathConstants<float>::pi * fc / static_cast<float>(sampleRate));
        float kf = 2.0f - 2.0f * 0.7f;  // moderate resonance on formant peaks
        float a1f = 1.0f / (1.0f + gf * (gf + kf));
        float a2f = gf * a1f;
        float a3f = gf * a2f;
        auto& s = formantBP[b];
        float v3 = input - s.ic2eq;
        float v1 = a1f * s.ic1eq + a2f * v3;
        float v2 = s.ic2eq + a2f * s.ic1eq + a3f * v3;
        s.ic1eq  = 2.0f * v1 - s.ic1eq;
        s.ic2eq  = 2.0f * v2 - s.ic2eq;
        out += v1; // BP output
    }
    return out * 0.5f;
}

float Filter::processSample(float input)
{
    // ── Even-harmonic input saturation — always on, ~2% ──────────────────────
    // Adds 2nd harmonic (x² term). Warms every patch without visible distortion.
    // Real analogue filters have this from the transistor/op-amp input stage.
    input = input + 0.022f * input * std::abs(input);   // asymmetric x·|x|
    input *= 0.978f;                                      // normalise gain

    if (drive > 0.0f)
        input = softClip(input);

    switch (type)
    {
        case FilterType::LP12: return applySVF(input, stage1, true);
        case FilterType::HP12: return applySVF(input, stage1, false);
        case FilterType::BP:
        {
            float v3 = input - stage1.ic2eq;
            float v1 = a1 * stage1.ic1eq + a2 * v3;
            float v2 = stage1.ic2eq + a2 * stage1.ic1eq + a3 * v3;
            stage1.ic1eq = 2.0f * v1 - stage1.ic1eq;
            stage1.ic2eq = 2.0f * v2 - stage1.ic2eq;
            return v1;
        }
        case FilterType::NOTCH:
        {
            float v3 = input - stage1.ic2eq;
            float v1 = a1 * stage1.ic1eq + a2 * v3;
            float v2 = stage1.ic2eq + a2 * stage1.ic1eq + a3 * v3;
            stage1.ic1eq = 2.0f * v1 - stage1.ic1eq;
            stage1.ic2eq = 2.0f * v2 - stage1.ic2eq;
            return input - k * v1;
        }
        case FilterType::LP24:
            return applyLadder(input);
        case FilterType::HP24:
        {
            float s1 = applySVF(input,  stage1, false);
            return    applySVF(s1,     stage2, false);
        }
        case FilterType::COMB:    return applyComb(input);
        case FilterType::FORMANT: return applyFormant(input);
    }
    return input;
}

void Filter::processBlock(float* data, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
        data[i] = processSample(data[i]);
}
