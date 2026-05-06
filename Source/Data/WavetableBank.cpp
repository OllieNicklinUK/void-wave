#include "WavetableBank.h"

WavetableBank::WavetableBank() = default;

// ── Helpers ───────────────────────────────────────────────────────────────────

static void normalize(float* data, int n)
{
    float peak = 0.0f;
    for (int i = 0; i < n; ++i) peak = std::max(peak, std::abs(data[i]));
    if (peak > 0.001f)
        for (int i = 0; i < n; ++i) data[i] /= peak;
}

// ── Table generation ─────────────────────────────────────────────────────────

void WavetableBank::loadBuiltInTables()
{
    addSineTable();
    addSawTable();
    addSquareTable();
    addSpectralTable();
    addFormantTable(0);   // Formant A → I
    addFormantTable(1);   // Formant E → O
    addBassTable(0);      // Bass 01
    addBassTable(1);      // Bass 02
    addNoiseTable();

    // Pre-allocate 4 user import slots (indices 9-12) with silence.
    // Audio thread always reads from these stable heap addresses; importUserTable()
    // fills them in-place so the pointer never changes.
    for (int i = 0; i < 4; ++i)
    {
        TableEntry e;
        e.name = "USR" + juce::String(i + 1);
        e.data.assign(static_cast<size_t>(SAMPLES_PER_TABLE), 0.0f);
        tables.push_back(std::move(e));
    }
}

// ── SINE: pure sine → harmonic-rich additive (morphs toward saw spectrum) ────

void WavetableBank::addSineTable()
{
    TableEntry e;
    e.name = "Sine";
    e.data.resize(static_cast<size_t>(SAMPLES_PER_TABLE));
    const float twoPi = juce::MathConstants<float>::twoPi;
    const int NH = 32;

    for (int frame = 0; frame < NUM_FRAMES; ++frame)
    {
        float t   = frame / static_cast<float>(NUM_FRAMES - 1);   // 0 → 1
        float* out = e.data.data() + frame * FRAME_SIZE;

        for (int s = 0; s < FRAME_SIZE; ++s)
        {
            float a = twoPi * s / FRAME_SIZE;
            float v = std::sin(a);   // fundamental always present
            for (int h = 2; h <= NH; ++h)
                v += (t * (1.0f / h)) * std::sin(h * a);   // harmonics grow with t
            out[s] = v;
        }
        normalize(out, FRAME_SIZE);
    }
    tables.push_back(std::move(e));
}

// ── SAW: sawtooth → square (even harmonics fade out with position) ────────────

void WavetableBank::addSawTable()
{
    TableEntry e;
    e.name = "Saw";
    e.data.resize(static_cast<size_t>(SAMPLES_PER_TABLE));
    const float twoPi = juce::MathConstants<float>::twoPi;
    const int NH = 48;

    for (int frame = 0; frame < NUM_FRAMES; ++frame)
    {
        float t   = frame / static_cast<float>(NUM_FRAMES - 1);
        float* out = e.data.data() + frame * FRAME_SIZE;

        for (int s = 0; s < FRAME_SIZE; ++s)
        {
            float a = twoPi * s / FRAME_SIZE;
            float v = 0.0f;
            for (int h = 1; h <= NH; ++h)
            {
                float amp = 1.0f / h;
                // Even harmonics fade toward zero → sound morphs from saw to square
                if (h % 2 == 0) amp *= (1.0f - t);
                v += amp * std::sin(h * a);
            }
            out[s] = v;
        }
        normalize(out, FRAME_SIZE);
    }
    tables.push_back(std::move(e));
}

// ── SQUARE: 50% pulse → 5% narrow pulse (full PWM sweep) ─────────────────────

void WavetableBank::addSquareTable()
{
    TableEntry e;
    e.name = "Square";
    e.data.resize(static_cast<size_t>(SAMPLES_PER_TABLE));

    for (int frame = 0; frame < NUM_FRAMES; ++frame)
    {
        float t  = frame / static_cast<float>(NUM_FRAMES - 1);
        float pw = 0.5f - t * 0.45f;   // 50% → 5% duty cycle
        float* out = e.data.data() + frame * FRAME_SIZE;
        int   edge = static_cast<int>(pw * FRAME_SIZE);

        for (int s = 0; s < FRAME_SIZE; ++s)
            out[s] = (s < edge) ? 1.0f : -1.0f;

        normalize(out, FRAME_SIZE);
    }
    tables.push_back(std::move(e));
}

// ── SPECTRAL: sweeping harmonic emphasis (peak harmonic rises with position) ──

void WavetableBank::addSpectralTable()
{
    TableEntry e;
    e.name = "Spectral";
    e.data.resize(static_cast<size_t>(SAMPLES_PER_TABLE));
    const float twoPi = juce::MathConstants<float>::twoPi;
    const int NH = 64;

    for (int frame = 0; frame < NUM_FRAMES; ++frame)
    {
        float t    = frame / static_cast<float>(NUM_FRAMES - 1);
        float peak = 1.0f + t * 30.0f;   // spectral peak sweeps from h=1 to h=31
        float* out = e.data.data() + frame * FRAME_SIZE;

        for (int s = 0; s < FRAME_SIZE; ++s)
        {
            float a = twoPi * s / FRAME_SIZE;
            float v = 0.0f;
            for (int h = 1; h <= NH; ++h)
            {
                float dist = static_cast<float>(h) - peak;
                float amp  = std::exp(-dist * dist * 0.06f)    // resonant peak
                           + (1.0f / h) * 0.25f;               // base overtone floor
                v += amp * std::sin(h * a);
            }
            out[s] = v;
        }
        normalize(out, FRAME_SIZE);
    }
    tables.push_back(std::move(e));
}

// ── FORMANT: vowel morph via shifting spectral peaks ─────────────────────────
// variant 0: vowel A (700,1100 Hz) → vowel I (270,2300 Hz)
// variant 1: vowel E (530,1840 Hz) → vowel O (570,840 Hz)

void WavetableBank::addFormantTable(int variant)
{
    TableEntry e;
    e.name = (variant == 0) ? "Formant A" : "Formant E";
    e.data.resize(static_cast<size_t>(SAMPLES_PER_TABLE));
    const float twoPi = juce::MathConstants<float>::twoPi;
    const int NH = 48;
    const float fund = 100.0f;   // reference fundamental for formant calc

    float F1s, F2s, F1e, F2e;
    if (variant == 0) { F1s = 700.f; F2s = 1100.f; F1e = 270.f;  F2e = 2300.f; }
    else              { F1s = 530.f; F2s = 1840.f; F1e = 570.f;  F2e =  840.f; }

    for (int frame = 0; frame < NUM_FRAMES; ++frame)
    {
        float t  = frame / static_cast<float>(NUM_FRAMES - 1);
        float F1 = F1s + t * (F1e - F1s);
        float F2 = F2s + t * (F2e - F2s);
        float* out = e.data.data() + frame * FRAME_SIZE;

        for (int s = 0; s < FRAME_SIZE; ++s)
        {
            float a = twoPi * s / FRAME_SIZE;
            float v = 0.0f;
            for (int h = 1; h <= NH; ++h)
            {
                float fh = h * fund;
                float g  = std::exp(-std::pow((fh - F1) / 200.0f, 2.0f))
                         + 0.6f * std::exp(-std::pow((fh - F2) / 300.0f, 2.0f));
                v += g * std::sin(h * a);
            }
            out[s] = v;
        }
        normalize(out, FRAME_SIZE);
    }
    tables.push_back(std::move(e));
}

// ── BASS: sub-heavy tone morphing into harmonically rich bass ─────────────────
// variant 0: sine + sub octave → saw-like bass
// variant 1: hollow (5th) → distorted bass

void WavetableBank::addBassTable(int variant)
{
    TableEntry e;
    e.name = (variant == 0) ? "Bass 01" : "Bass 02";
    e.data.resize(static_cast<size_t>(SAMPLES_PER_TABLE));
    const float twoPi = juce::MathConstants<float>::twoPi;
    const int NH = 32;

    for (int frame = 0; frame < NUM_FRAMES; ++frame)
    {
        float t   = frame / static_cast<float>(NUM_FRAMES - 1);
        float* out = e.data.data() + frame * FRAME_SIZE;

        for (int s = 0; s < FRAME_SIZE; ++s)
        {
            float a = twoPi * s / FRAME_SIZE;
            float v = 0.0f;

            if (variant == 0)
            {
                // Starts as pure sine + growing sub octave; harmonics accumulate
                v  = std::sin(a);                            // fundamental
                v += std::sin(a * 0.5f) * t * 0.7f;         // sub octave grows in
                v += std::sin(a * 2.0f) * t * 0.4f;         // 2nd harmonic adds grit
                v += std::sin(a * 3.0f) * t * 0.2f;         // 3rd
                v += std::sin(a * 4.0f) * t * 0.12f;        // 4th
            }
            else
            {
                // Starts hollow (root + 5th); saw harmonics emerge with position
                v  = std::sin(a)         * (1.0f - t * 0.2f);   // root
                v += std::sin(a * 1.5f) * (1.0f - t);           // 5th fades out
                for (int h = 2; h <= NH; ++h)
                    v += (t / h) * std::exp(-h * 0.06f) * std::sin(h * a);
            }
            out[s] = v;
        }
        normalize(out, FRAME_SIZE);
    }
    tables.push_back(std::move(e));
}

// ── NOISE: pseudo-random spectral texture (pink → white noise profile) ────────

void WavetableBank::addNoiseTable()
{
    TableEntry e;
    e.name = "Noise";
    e.data.resize(static_cast<size_t>(SAMPLES_PER_TABLE));
    const float twoPi = juce::MathConstants<float>::twoPi;
    const int NH = 48;

    for (int frame = 0; frame < NUM_FRAMES; ++frame)
    {
        float t = frame / static_cast<float>(NUM_FRAMES - 1);

        // Pre-compute phases for this frame — seeded so each frame is unique
        juce::Random rng(static_cast<juce::int64>(frame * 1234567LL + 987654LL));
        std::vector<float> phases(static_cast<size_t>(NH));
        std::vector<float> amps  (static_cast<size_t>(NH));
        for (int h = 0; h < NH; ++h)
        {
            phases[static_cast<size_t>(h)] = rng.nextFloat() * twoPi;
            // Spectral slope: -1.5 (pink) → -0.5 (brighter) as t increases
            amps[static_cast<size_t>(h)] = std::pow(static_cast<float>(h + 1), -(1.5f - t));
        }

        float* out = e.data.data() + frame * FRAME_SIZE;
        for (int s = 0; s < FRAME_SIZE; ++s)
        {
            float a = twoPi * s / FRAME_SIZE;
            float v = 0.0f;
            for (int h = 0; h < NH; ++h)
                v += amps[static_cast<size_t>(h)]
                   * std::sin((h + 1) * a + phases[static_cast<size_t>(h)]);
            out[s] = v;
        }
        normalize(out, FRAME_SIZE);
    }
    tables.push_back(std::move(e));
}

// ── Info ──────────────────────────────────────────────────────────────────────

const juce::String& WavetableBank::getTableName(int index) const
{
    if (index >= 0 && index < static_cast<int>(tables.size()))
        return tables[static_cast<size_t>(index)].name;
    return defaultName;
}

const float* WavetableBank::getTableData(int index) const
{
    if (index >= 0 && index < static_cast<int>(tables.size()))
        return tables[static_cast<size_t>(index)].data.data();
    return nullptr;
}

bool WavetableBank::importUserTable(int userSlot, const juce::File& wavFile)
{
    int targetIdx = 9 + userSlot;
    if (targetIdx < 0 || targetIdx >= static_cast<int>(tables.size()))
        return false;

    juce::AudioFormatManager mgr;
    mgr.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(mgr.createReaderFor(wavFile));
    if (!reader || reader->lengthInSamples < 8)
        return false;

    int numSrc = static_cast<int>(juce::jmin((juce::int64)reader->lengthInSamples, (juce::int64)reader->sampleRate));
    juce::AudioBuffer<float> src(1, numSrc);
    reader->read(&src, 0, numSrc, 0, true, false);
    const float* s = src.getReadPointer(0);

    // Resample one cycle to FRAME_SIZE via linear interpolation
    std::vector<float> oneFrame(static_cast<size_t>(FRAME_SIZE));
    double ratio = static_cast<double>(numSrc) / FRAME_SIZE;
    for (int i = 0; i < FRAME_SIZE; ++i)
    {
        double pos  = i * ratio;
        int    lo   = static_cast<int>(pos) % numSrc;
        int    hi   = (lo + 1) % numSrc;
        float  frac = static_cast<float>(pos - std::floor(pos));
        oneFrame[static_cast<size_t>(i)] = s[lo] + frac * (s[hi] - s[lo]);
    }

    // Normalise
    float peak = 0.0f;
    for (float v : oneFrame) peak = std::max(peak, std::abs(v));
    if (peak > 0.001f) for (float& v : oneFrame) v /= peak;

    // Tile into 256 identical frames (in-place — pointer stays valid)
    auto& data = tables[static_cast<size_t>(targetIdx)].data;
    for (int f = 0; f < NUM_FRAMES; ++f)
        std::copy(oneFrame.begin(), oneFrame.end(),
                  data.begin() + static_cast<ptrdiff_t>(f * FRAME_SIZE));

    tables[static_cast<size_t>(targetIdx)].name =
        wavFile.getFileNameWithoutExtension().substring(0, 6).toUpperCase();
    return true;
}
