#include "WavetableBank.h"

WavetableBank::WavetableBank() = default;

void WavetableBank::loadBuiltInTables()
{
    // Stub: generate a handful of analytic tables so the plugin makes sound.
    // Stage 3 will replace these with full 256-frame wavetable sets.
    addSineTable();
    addSawTable();
    addSquareTable();
}

void WavetableBank::addSineTable()
{
    TableEntry e;
    e.name = "Sine";
    e.data.resize(static_cast<size_t>(SAMPLES_PER_TABLE));
    for (int frame = 0; frame < NUM_FRAMES; ++frame)
        for (int s = 0; s < FRAME_SIZE; ++s)
            e.data[static_cast<size_t>(frame * FRAME_SIZE + s)] =
                std::sin(2.0f * juce::MathConstants<float>::pi
                         * static_cast<float>(s) / static_cast<float>(FRAME_SIZE));
    tables.push_back(std::move(e));
}

void WavetableBank::addSawTable()
{
    TableEntry e;
    e.name = "Saw";
    e.data.resize(static_cast<size_t>(SAMPLES_PER_TABLE));
    for (int frame = 0; frame < NUM_FRAMES; ++frame)
        for (int s = 0; s < FRAME_SIZE; ++s)
            e.data[static_cast<size_t>(frame * FRAME_SIZE + s)] =
                2.0f * static_cast<float>(s) / static_cast<float>(FRAME_SIZE) - 1.0f;
    tables.push_back(std::move(e));
}

void WavetableBank::addSquareTable()
{
    TableEntry e;
    e.name = "Square";
    e.data.resize(static_cast<size_t>(SAMPLES_PER_TABLE));
    for (int frame = 0; frame < NUM_FRAMES; ++frame)
        for (int s = 0; s < FRAME_SIZE; ++s)
            e.data[static_cast<size_t>(frame * FRAME_SIZE + s)] =
                (s < FRAME_SIZE / 2) ? 1.0f : -1.0f;
    tables.push_back(std::move(e));
}

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

bool WavetableBank::importUserTable(int /*userSlot*/, const juce::File& /*wavFile*/)
{
    // TODO: Stage 3 — parse WAV, resample to FRAME_SIZE, store in user slot
    return false;
}
