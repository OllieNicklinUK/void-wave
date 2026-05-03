#include "ModMatrix.h"

ModMatrix::ModMatrix() = default;

void ModMatrix::setSlot(int slot, ModSource src, ModDest dst, float depth, bool enabled)
{
    if (slot < 0 || slot >= NUM_SLOTS) return;
    slots[slot] = { src, dst, depth, enabled };
}

void ModMatrix::clearSlot(int slot)
{
    if (slot < 0 || slot >= NUM_SLOTS) return;
    slots[slot] = {};
}

void ModMatrix::process(ModValues& mv) const
{
    // Reset dest accumulator
    for (auto& d : mv.dests) d = 0.0f;

    for (const auto& s : slots)
    {
        if (!s.enabled) continue;
        int srcIdx = static_cast<int>(s.source);
        int dstIdx = static_cast<int>(s.dest);
        mv.dests[dstIdx] += mv.sources[srcIdx] * s.depth;
    }
}
