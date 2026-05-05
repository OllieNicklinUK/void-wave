#include "Reverb.h"

void VWReverb::prepare(double sr, int /*blockSize*/)
{
    sampleRate = sr;
    dt         = 1.0 / sr;
    t          = 0.0f;

    late  = std::make_unique<LateReverb>(static_cast<int>(sr));
    early = std::make_unique<EarlyReflections>(static_cast<int>(sr));

    late->gain  = 0.15f + size * 0.73f;   // 0-1 → 0.15-0.88 (below self-oscillation)
    dampStateL  = dampStateR = 0.0f;

    predelayBufL.assign(MAX_PREDELAY, 0.0f);
    predelayBufR.assign(MAX_PREDELAY, 0.0f);
    predelayPos     = 0;
    predelaySamples = juce::jlimit(0, MAX_PREDELAY - 1,
                                   static_cast<int>(predelayMs * sr / 1000.0));
}

void VWReverb::reset()
{
    dampStateL = dampStateR = t = 0.0f;
    if (!predelayBufL.empty()) std::fill(predelayBufL.begin(), predelayBufL.end(), 0.0f);
    if (!predelayBufR.empty()) std::fill(predelayBufR.begin(), predelayBufR.end(), 0.0f);
    predelayPos = 0;
}

void VWReverb::setSize(float s)
{
    size = s;
    if (late) late->gain = 0.15f + s * 0.73f;
}

void VWReverb::setDamping(float d)  { damping = d; }
void VWReverb::setWidth(float w)    { width   = w; }
void VWReverb::setMix(float m)      { mix     = m; }
void VWReverb::setEnabled(bool e)   { enabled = e; }

void VWReverb::setPredelay(float ms)
{
    predelayMs      = ms;
    predelaySamples = juce::jlimit(0, MAX_PREDELAY - 1,
                                   static_cast<int>(ms * sampleRate / 1000.0));
}

void VWReverb::process(juce::AudioBuffer<float>& buffer)
{
    if (!enabled || !late || mix < 0.001f)
        return;

    const int numCh  = buffer.getNumChannels();
    const int numSmp = buffer.getNumSamples();
    auto* L = buffer.getWritePointer(0);
    auto* R = numCh > 1 ? buffer.getWritePointer(1) : L;

    // damping=0 → bright (coeff≈1.0),  damping=1 → dark (coeff≈0.05)
    const float dampCoeff = 1.0f - damping * 0.95f;

    for (int i = 0; i < numSmp; ++i)
    {
        float inL = L[i], inR = R[i];

        // Optional pre-delay
        float dL = inL, dR = inR;
        if (predelaySamples > 0)
        {
            int rd = (predelayPos - predelaySamples + MAX_PREDELAY) % MAX_PREDELAY;
            dL = predelayBufL[static_cast<size_t>(rd)];
            dR = predelayBufR[static_cast<size_t>(rd)];
            predelayBufL[static_cast<size_t>(predelayPos)] = inL;
            predelayBufR[static_cast<size_t>(predelayPos)] = inR;
            predelayPos = (predelayPos + 1) % MAX_PREDELAY;
        }

        // Early reflections — add initial space and punch
        early->process(dL, dR);

        // Late reverb (Peverb time-varying nested all-pass)
        float mono = (dL + dR) * 0.5f;
        late->process(t, mono, mono);

        // Post damping one-pole LP
        dampStateL += dampCoeff * (late->left  - dampStateL);
        dampStateR += dampCoeff * (late->right - dampStateR);

        float wetL = dampStateL + early->left  * 0.2f;
        float wetR = dampStateR + early->right * 0.2f;

        // Width
        if (width < 0.999f)
        {
            float mid = (wetL + wetR) * 0.5f;
            wetL = mid + (wetL - mid) * width;
            wetR = mid + (wetR - mid) * width;
        }

        L[i] = inL * (1.0f - mix) + wetL * mix;
        R[i] = inR * (1.0f - mix) + wetR * mix;

        t += static_cast<float>(dt);
    }
}
