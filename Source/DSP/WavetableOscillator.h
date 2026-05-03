#pragma once
#include <JuceHeader.h>

enum class PhaseMode  { FREE, FIXED, RANDOM };
enum class WTMode     { SINGLE, MORPH, SCAN  };

// Single-voice wavetable oscillator.
// Format: 256 frames × 2048 samples, float32, linear interpolation between frames.
// 4× oversampling internally, downsampled on output.
class WavetableOscillator
{
public:
    WavetableOscillator();

    void prepare(double sampleRate, int blockSize);
    void reset();

    void setFrequency(float hz);
    void setTableIndex(int index);          // 0–63
    void setTableData(const float* data);   // pointer into WavetableBank (not owned)
    void setPosition(float pos);            // 0.0–1.0 frame position
    void setCoarse(int semitones);          // -24 to +24
    void setFine(float cents);              // -100 to +100
    void setLevel(float level);             // 0.0–1.0
    void setPan(float pan);                 // -1.0–1.0
    void setPhaseMode(PhaseMode mode);
    void setWTMode(WTMode mode);
    void setPhase(float normalised);        // 0.0–1.0, used in FIXED mode
    void triggerNote();

    // Fill left/right output at base sample rate (after internal downsampling).
    void process(float* outL, float* outR, int numSamples);

    // For WavetableDisplay: returns current rendered frame samples (2048 pts).
    const std::vector<float>& getCurrentFrameSamples() const { return displayFrame; }

private:
    static constexpr int OVERSAMPLE = 4;
    static constexpr int FRAME_SIZE = 2048;
    static constexpr int NUM_FRAMES = 256;

    double sampleRate  = 44100.0;
    float  baseFreq    = 440.0f;
    float  position    = 0.0f;
    float  level       = 1.0f;
    float  panL        = 1.0f;
    float  panR        = 1.0f;
    float  phase       = 0.0f;   // 0.0–1.0 accumulator
    float  phaseInc    = 0.0f;   // per oversampled sample
    float  fixedPhase  = 0.0f;

    int       tableIndex = 0;
    PhaseMode phaseMode  = PhaseMode::FREE;
    WTMode    wtMode     = WTMode::SINGLE;

    // SCAN mode: auto-advance wavetable position. position knob = scan rate (0–4 Hz).
    float scanPhase    = 0.0f;  // 0.0–1.0 wrapping scan position

    // Wavetable data pointer (owned by WavetableBank, not this class)
    const float* tableData = nullptr;

    std::vector<float> displayFrame;

    float interpolateSample(float framePos, float phasePos) const;
    void  updatePhaseInc();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavetableOscillator)
};
