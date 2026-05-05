#include "UnisonVoice.h"

UnisonVoice::UnisonVoice() = default;

void UnisonVoice::prepare(double sr, int blockSize)
{
    sampleRate = sr;
    for (auto& v : voices) v.prepare(sr, blockSize);
    initMicroMod();
}

void UnisonVoice::reset()
{
    for (auto& v : voices) v.reset();
}

void UnisonVoice::initMicroMod()
{
    // Each unison sub-voice gets its own independent slow LFO.
    // Rates: 0.01–0.25 Hz — too slow to hear as vibrato, but the beating
    // pattern never repeats, making wide unison feel alive and expensive.
    for (int i = 0; i < MAX_VOICES; ++i)
    {
        float rate = 0.01f + rng.nextFloat() * 0.24f;
        microMod[i].phaseInc   = rate / static_cast<float>(sampleRate);
        microMod[i].phase      = rng.nextFloat();               // random initial phase
        microMod[i].pitchCents = 0.4f + rng.nextFloat() * 2.1f; // 0.4–2.5 cents
        microMod[i].levelAmt   = 0.005f + rng.nextFloat() * 0.015f; // 0.5–2%
    }
}

void UnisonVoice::setFrequency(float hz) { baseFreq = hz; applyDetuneAndPan(); }
void UnisonVoice::setNumVoices(int n)    { numVoices = juce::jlimit(1, MAX_VOICES, n); applyDetuneAndPan(); }
void UnisonVoice::setDetune(float a)     { detune = juce::jlimit(0.0f, 1.0f, a); applyDetuneAndPan(); }
void UnisonVoice::setWidth(float w)      { width  = juce::jlimit(0.0f, 1.0f, w); applyDetuneAndPan(); }

void UnisonVoice::setTableIndex(int i) { for (auto& v : voices) v.setTableIndex(i); }
void UnisonVoice::setTableData(const float* d) { for (auto& v : voices) v.setTableData(d); }
void UnisonVoice::setPosition(float p) { for (auto& v : voices) v.setPosition(p); }
void UnisonVoice::setLevel(float l)    { for (auto& v : voices) v.setLevel(l); }
void UnisonVoice::setPhaseMode(PhaseMode m) { for (auto& v : voices) v.setPhaseMode(m); }
void UnisonVoice::setWTMode(WTMode m)       { for (auto& v : voices) v.setWTMode(m); }

void UnisonVoice::triggerNote()
{
    initMicroMod();  // re-randomise on each note so the drift pattern is never the same
    for (auto& v : voices) v.triggerNote();
}

void UnisonVoice::applyDetuneAndPan()
{
    float spreadCents = detune * 200.0f;  // 0–200 cents spread (audible at any setting)
    for (int i = 0; i < numVoices; ++i)
    {
        float t = (numVoices > 1)
                  ? static_cast<float>(i) / static_cast<float>(numVoices - 1)
                  : 0.5f;
        float centOffset = spreadCents * (t - 0.5f);
        voiceFreqs[i] = baseFreq * std::pow(2.0f, centOffset / 1200.0f);
        voices[i].setFrequency(voiceFreqs[i]);

        // Width scales the pan spread: width=0 → all centre, width=1 → full L/R
        float pan = (numVoices > 1) ? width * (t * 2.0f - 1.0f) : 0.0f;
        voices[i].setPan(pan);
    }
}

void UnisonVoice::process(float* outL, float* outR, int numSamples)
{
    std::fill(outL, outL + numSamples, 0.0f);
    std::fill(outR, outR + numSamples, 0.0f);

    std::vector<float> tmpL(static_cast<size_t>(numSamples));
    std::vector<float> tmpR(static_cast<size_t>(numSamples));

    const float norm = 1.0f / static_cast<float>(numVoices);

    for (int i = 0; i < numVoices; ++i)
    {
        // ── Advance micro LFO (per block, fine at these slow rates) ──────────
        microMod[i].phase += microMod[i].phaseInc * static_cast<float>(numSamples);
        if (microMod[i].phase >= 1.0f) microMod[i].phase -= 1.0f;
        float lfo = std::sin(microMod[i].phase * juce::MathConstants<float>::twoPi);

        // ── Micro pitch drift — apply on top of static detune ─────────────────
        float microPitchMult = std::pow(2.0f, lfo * microMod[i].pitchCents / 1200.0f);
        if (voiceFreqs[i] > 0.0f)
            voices[i].setFrequency(voiceFreqs[i] * microPitchMult);

        voices[i].process(tmpL.data(), tmpR.data(), numSamples);

        // ── Micro level drift — each voice breathes slightly ──────────────────
        float levelScale = norm * (1.0f + lfo * microMod[i].levelAmt);
        for (int s = 0; s < numSamples; ++s)
        {
            outL[s] += tmpL[static_cast<size_t>(s)] * levelScale;
            outR[s] += tmpR[static_cast<size_t>(s)] * levelScale;
        }
    }
}
