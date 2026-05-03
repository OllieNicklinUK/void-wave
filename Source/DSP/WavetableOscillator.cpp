#include "WavetableOscillator.h"

WavetableOscillator::WavetableOscillator()
{
    displayFrame.resize(FRAME_SIZE, 0.0f);
}

void WavetableOscillator::prepare(double sr, int /*blockSize*/)
{
    sampleRate = sr;
    updatePhaseInc();
}

void WavetableOscillator::reset()
{
    phase = (phaseMode == PhaseMode::FIXED) ? fixedPhase
          : (phaseMode == PhaseMode::RANDOM) ? juce::Random::getSystemRandom().nextFloat()
          : 0.0f;
}

void WavetableOscillator::setFrequency(float hz)   { baseFreq = hz; updatePhaseInc(); }
void WavetableOscillator::setTableIndex(int i)      { tableIndex = juce::jlimit(0, 63, i); }
void WavetableOscillator::setTableData(const float* data) { tableData = data; }
void WavetableOscillator::setPosition(float p)      { position = juce::jlimit(0.0f, 1.0f, p); }
void WavetableOscillator::setLevel(float l)         { level = l; }
void WavetableOscillator::setPhaseMode(PhaseMode m) { phaseMode = m; }
void WavetableOscillator::setWTMode(WTMode m)       { wtMode = m; }
void WavetableOscillator::setPhase(float n)         { fixedPhase = n; }

void WavetableOscillator::setCoarse(int semitones)
{
    // Applied externally via frequency calculation in VoiceManager / UnisonVoice
    juce::ignoreUnused(semitones);
}

void WavetableOscillator::setFine(float cents)
{
    juce::ignoreUnused(cents);
}

void WavetableOscillator::setPan(float pan)
{
    // Constant-power pan
    float angle = (pan + 1.0f) * 0.5f * juce::MathConstants<float>::halfPi;
    panL = std::cos(angle);
    panR = std::sin(angle);
}

void WavetableOscillator::triggerNote()
{
    reset();
}

void WavetableOscillator::updatePhaseInc()
{
    // Phase increment per oversampled sample
    phaseInc = baseFreq / static_cast<float>(sampleRate * OVERSAMPLE);
}

float WavetableOscillator::interpolateSample(float framePos, float phasePos) const
{
    if (tableData == nullptr)
        return 0.0f;

    // framePos: 0.0–255.0 (fractional frame index)
    int   frameA = static_cast<int>(framePos) % NUM_FRAMES;
    int   frameB = (frameA + 1) % NUM_FRAMES;
    float frameFrac = framePos - static_cast<float>(static_cast<int>(framePos));

    // phasePos: 0.0–1.0
    float samplePosF = phasePos * static_cast<float>(FRAME_SIZE);
    int   sampleA    = static_cast<int>(samplePosF) % FRAME_SIZE;
    int   sampleB    = (sampleA + 1) % FRAME_SIZE;
    float sampleFrac = samplePosF - static_cast<float>(static_cast<int>(samplePosF));

    auto read = [&](int frame, int sample) -> float {
        return tableData[(frame * FRAME_SIZE) + sample];
    };

    float sA = read(frameA, sampleA) + sampleFrac * (read(frameA, sampleB) - read(frameA, sampleA));
    float sB = read(frameB, sampleA) + sampleFrac * (read(frameB, sampleB) - read(frameB, sampleA));
    return sA + frameFrac * (sB - sA);
}

void WavetableOscillator::process(float* outL, float* outR, int numSamples)
{
    if (tableData == nullptr)
    {
        for (int i = 0; i < numSamples; ++i) outL[i] = outR[i] = 0.0f;
        return;
    }

    const float scanRateHz   = position * 4.0f;
    const float scanPhaseInc = scanRateHz / static_cast<float>(sampleRate);

    for (int i = 0; i < numSamples; ++i)
    {
        float framePos;
        if (wtMode == WTMode::SCAN)
        {
            scanPhase += scanPhaseInc;
            if (scanPhase >= 1.0f) scanPhase -= 1.0f;
            framePos = scanPhase * static_cast<float>(NUM_FRAMES - 1);
        }
        else
        {
            framePos = position * static_cast<float>(NUM_FRAMES - 1);
        }

        float sum = 0.0f;
        for (int k = 0; k < OVERSAMPLE; ++k)
        {
            sum += interpolateSample(framePos, phase);
            phase += phaseInc;
            if (phase >= 1.0f) phase -= 1.0f;
        }
        float s = (sum / static_cast<float>(OVERSAMPLE)) * level;
        outL[i] = s * panL;
        outR[i] = s * panR;
    }
}
