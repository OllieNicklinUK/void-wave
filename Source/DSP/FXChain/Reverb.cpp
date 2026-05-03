#include "Reverb.h"

// Freeverb tuning (samples @ 44100 Hz)
const int VWReverb::COMB_TUNING_L[NUM_COMBS]   = {1116,1188,1277,1356,1422,1491,1557,1617};
const int VWReverb::COMB_TUNING_R[NUM_COMBS]   = {1139,1211,1300,1379,1445,1514,1580,1640};
const int VWReverb::AP_TUNING_L[NUM_ALLPASS]   = {556, 441, 341, 225};
const int VWReverb::AP_TUNING_R[NUM_ALLPASS]   = {579, 464, 364, 248};

// ── CombFilter ────────────────────────────────────────────────────────────────

void VWReverb::CombFilter::prepare(int sz)
{
    buf.assign(static_cast<size_t>(sz), 0.0f);
    writePos  = 0;
    dampState = 0.0f;
}

void VWReverb::CombFilter::reset()
{
    std::fill(buf.begin(), buf.end(), 0.0f);
    writePos  = 0;
    dampState = 0.0f;
}

float VWReverb::CombFilter::process(float input)
{
    int   sz  = static_cast<int>(buf.size());
    float out = buf[static_cast<size_t>(writePos)];
    dampState = out * (1.0f - dampCoeff) + dampState * dampCoeff;
    buf[static_cast<size_t>(writePos)] = input + dampState * feedback;
    writePos  = (writePos + 1) % sz;
    return out;
}

// ── AllPassFilter ─────────────────────────────────────────────────────────────

void VWReverb::AllPassFilter::prepare(int sz)
{
    buf.assign(static_cast<size_t>(sz), 0.0f);
    writePos = 0;
}

void VWReverb::AllPassFilter::reset()
{
    std::fill(buf.begin(), buf.end(), 0.0f);
    writePos = 0;
}

float VWReverb::AllPassFilter::process(float input)
{
    int   sz      = static_cast<int>(buf.size());
    float delayed = buf[static_cast<size_t>(writePos)];
    float output  = -input + delayed;
    buf[static_cast<size_t>(writePos)] = input + delayed * feedback;
    writePos = (writePos + 1) % sz;
    return output;
}

// ── Reverb ───────────────────────────────────────────────────────────────────

VWReverb::VWReverb() = default;

int VWReverb::scaleToSampleRate(int base) const
{
    return static_cast<int>(static_cast<double>(base) * sampleRate / 44100.0);
}

void VWReverb::prepare(double sr, int /*blockSize*/)
{
    sampleRate = sr;
    predelayBuf.assign(MAX_PREDELAY, 0.0f);

    for (int i = 0; i < NUM_COMBS; ++i)
    {
        combL[i].prepare(scaleToSampleRate(COMB_TUNING_L[i]));
        combR[i].prepare(scaleToSampleRate(COMB_TUNING_R[i]));
    }
    for (int i = 0; i < NUM_ALLPASS; ++i)
    {
        allPassL[i].prepare(scaleToSampleRate(AP_TUNING_L[i]));
        allPassR[i].prepare(scaleToSampleRate(AP_TUNING_R[i]));
    }
    updateParams();
}

void VWReverb::reset()
{
    for (auto& c : combL) c.reset();
    for (auto& c : combR) c.reset();
    for (auto& a : allPassL) a.reset();
    for (auto& a : allPassR) a.reset();
    std::fill(predelayBuf.begin(), predelayBuf.end(), 0.0f);
    predelayPos = 0;
}

void VWReverb::setSize(float s)     { size    = s; updateParams(); }
void VWReverb::setDamping(float d)  { damping = d; updateParams(); }
void VWReverb::setWidth(float w)    { width   = w; }
void VWReverb::setMix(float m)      { mix     = m; }
void VWReverb::setEnabled(bool on)  { enabled = on; }

void VWReverb::setPredelay(float ms)
{
    predelaySamples = juce::jlimit(0,
        MAX_PREDELAY - 1,
        static_cast<int>(ms * sampleRate * 0.001));
}

void VWReverb::updateParams()
{
    float roomFeedback = 0.5f + size * 0.48f;
    float dampCoeff    = damping * 0.4f;

    for (auto& c : combL) { c.feedback = roomFeedback; c.dampCoeff = dampCoeff; }
    for (auto& c : combR) { c.feedback = roomFeedback; c.dampCoeff = dampCoeff; }
}

void VWReverb::process(juce::AudioBuffer<float>& buffer)
{
    if (!enabled) return;

    auto* L = buffer.getWritePointer(0);
    auto* R = buffer.getWritePointer(1);
    int   n = buffer.getNumSamples();

    for (int i = 0; i < n; ++i)
    {
        float dryL = L[i], dryR = R[i];
        float mono = (dryL + dryR) * 0.5f;

        // Pre-delay
        float delayed = predelayBuf[static_cast<size_t>(predelayPos)];
        predelayBuf[static_cast<size_t>(predelayPos)] = mono;
        predelayPos = (predelayPos + 1) % MAX_PREDELAY;
        float input = (predelaySamples > 0) ? delayed : mono;

        // 8 parallel comb filters
        float outL = 0.0f, outR = 0.0f;
        for (int c = 0; c < NUM_COMBS; ++c)
        {
            outL += combL[c].process(input);
            outR += combR[c].process(input);
        }

        // 4 series all-pass filters
        for (int a = 0; a < NUM_ALLPASS; ++a)
        {
            outL = allPassL[a].process(outL);
            outR = allPassR[a].process(outR);
        }

        // Stereo width matrix
        float wetL = outL + width * (outR - outL) * 0.5f;
        float wetR = outR + width * (outL - outR) * 0.5f;

        L[i] = dryL + mix * wetL;
        R[i] = dryR + mix * wetR;
    }
}
