#pragma once

#include <cstddef>

#include <array>
#include <cmath>
#include <cstdint>

namespace nscm
{
// Maximum resonator voices (spec §18.3 High quality).
inline constexpr int kMaxVoices = 32;

// MIDI note -> frequency in Hz.
inline float midiNoteToHz (float note) noexcept
{
    return 440.0f * std::pow (2.0f, (note - 69.0f) / 12.0f);
}

// Holds the resonator target frequencies for the current chord / scale.
struct TargetNoteList
{
    std::array<float, kMaxVoices> freqs {};
    std::array<int, kMaxVoices>   notes {}; // MIDI note backing each freq (for UI)
    int count = 0;
};

// Build the target list from a 12-bit pitch-class mask, expanded across octaves
// (spec §14.2). scaleShift transposes everything by semitones. The musical range
// is sampled evenly so the bank spreads colour across octaves rather than piling
// up in the bass.
inline void fillTargetNotes (TargetNoteList& out, std::uint16_t mask, int scaleShift,
                             int maxVoices) noexcept
{
    out.count = 0;

    if (mask == 0)
        return;

    maxVoices = maxVoices < 1 ? 1 : (maxVoices > kMaxVoices ? kMaxVoices : maxVoices);

    // Candidate MIDI range: C2 (36) .. C7 (96).
    constexpr int lowNote  = 36;
    constexpr int highNote = 96;

    std::array<int, highNote - lowNote + 1> candidates {};
    int candidateCount = 0;

    for (int n = lowNote; n <= highNote; ++n)
    {
        const int pc = ((n % 12) + 12) % 12;
        if (mask & (1u << pc))
            candidates[(std::size_t) candidateCount++] = n + scaleShift;
    }

    if (candidateCount == 0)
        return;

    if (candidateCount <= maxVoices)
    {
        for (int i = 0; i < candidateCount; ++i)
        {
            out.notes[(std::size_t) i] = candidates[(std::size_t) i];
            out.freqs[(std::size_t) i] = midiNoteToHz ((float) candidates[(std::size_t) i]);
        }
        out.count = candidateCount;
        return;
    }

    // Sub-sample evenly across the candidate set.
    for (int i = 0; i < maxVoices; ++i)
    {
        const int idx = (int) std::lround ((double) i * (candidateCount - 1) / (maxVoices - 1));
        out.notes[(std::size_t) i] = candidates[(std::size_t) idx];
        out.freqs[(std::size_t) i] = midiNoteToHz ((float) candidates[(std::size_t) idx]);
    }
    out.count = maxVoices;
}
} // namespace nscm
