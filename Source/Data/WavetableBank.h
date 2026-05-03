#pragma once
#include <JuceHeader.h>

// Owns all wavetable data in memory.
// Format: 256 frames × 2048 samples × float32 per table.
// WavetableOscillator holds a const pointer into this bank's memory.
class WavetableBank
{
public:
    static constexpr int NUM_FRAMES      = 256;
    static constexpr int FRAME_SIZE      = 2048;
    static constexpr int SAMPLES_PER_TABLE = NUM_FRAMES * FRAME_SIZE;

    WavetableBank();

    // Load built-in tables from BinaryData (called once at startup)
    void loadBuiltInTables();

    // Returns total number of tables loaded
    int getNumTables() const { return static_cast<int>(tables.size()); }

    // Returns display name for table at index
    const juce::String& getTableName(int index) const;

    // Returns raw pointer for WavetableOscillator (must outlive the oscillator)
    const float* getTableData(int index) const;

    // Drag-drop WAV import (up to 4 user slots)
    bool importUserTable(int userSlot, const juce::File& wavFile);

private:
    struct TableEntry
    {
        juce::String name;
        std::vector<float> data; // NUM_FRAMES * FRAME_SIZE floats
    };

    std::vector<TableEntry> tables;
    juce::String            defaultName { "---" };

    void addSineTable();
    void addSawTable();
    void addSquareTable();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavetableBank)
};
