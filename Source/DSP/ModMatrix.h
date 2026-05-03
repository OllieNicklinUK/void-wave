#pragma once
#include <JuceHeader.h>

// Sources (index into ModValues::sources[])
enum class ModSource : int
{
    ENV1 = 0, ENV2, ENV3,
    LFO1, LFO2,
    VELOCITY, NOTE, AFTERTOUCH,
    MOD_WHEEL, BREATH,
    MACRO1, MACRO2, MACRO3, MACRO4,
    MIDI_CC_X,
    NUM_SOURCES
};

// Destinations
enum class ModDest : int
{
    OSC1_PITCH = 0, OSC2_PITCH,
    OSC1_WT_POS,    OSC2_WT_POS,
    OSC1_LEVEL,     OSC2_LEVEL,
    OSC1_PAN,       OSC2_PAN,
    OSC1_UNI_DETUNE, OSC2_UNI_DETUNE,
    FILTER_CUTOFF,  FILTER_RES, FILTER_DRIVE,
    LFO1_RATE, LFO2_RATE,
    LFO1_DEPTH, LFO2_DEPTH,
    ENV1_DEPTH,
    AMP,
    NUM_DESTS
};

struct ModSlot
{
    ModSource source  = ModSource::ENV1;
    ModDest   dest    = ModDest::OSC1_PITCH;
    float     depth   = 0.0f;   // -1.0–1.0
    bool      enabled = false;
};

// Accumulated modulation values per destination, evaluated once per buffer.
struct ModValues
{
    float sources[static_cast<int>(ModSource::NUM_SOURCES)] = {};
    float dests  [static_cast<int>(ModDest::NUM_DESTS)]     = {};
};

class ModMatrix
{
public:
    static constexpr int NUM_SLOTS = 12;

    ModMatrix();

    void setSlot(int slot, ModSource src, ModDest dst, float depth, bool enabled);
    void clearSlot(int slot);

    // Call once per buffer. Fills dests[] from sources[] already populated by caller.
    void process(ModValues& mv) const;

    const ModSlot& getSlot(int i) const { return slots[i]; }

private:
    ModSlot slots[NUM_SLOTS];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModMatrix)
};
