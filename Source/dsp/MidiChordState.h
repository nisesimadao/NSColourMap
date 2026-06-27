#pragma once

#include <array>
#include <cstdint>

namespace nscm
{
// Tracks currently-held MIDI notes and, optionally, freezes the last chord
// after all notes are released (spec §14). Audio-thread safe: no allocation.
class MidiChordState
{
public:
    void reset() noexcept
    {
        held.fill (false);
        heldCount = 0;
        frozenMask = 0;
        liveMask = 0;
        lowestHeldNote = -1;
    }

    void noteOn (int note) noexcept
    {
        if (note < 0 || note > 127)
            return;

        if (! held[(size_t) note])
        {
            held[(size_t) note] = true;
            ++heldCount;
        }

        rebuildLiveMask();
        frozenMask = liveMask; // keep frozen chord up to date with newest input
    }

    void noteOff (int note) noexcept
    {
        if (note < 0 || note > 127)
            return;

        if (held[(size_t) note])
        {
            held[(size_t) note] = false;
            --heldCount;
        }

        rebuildLiveMask();
    }

    void allNotesOff() noexcept
    {
        held.fill (false);
        heldCount = 0;
        liveMask = 0;
        lowestHeldNote = -1;
    }

    bool hasHeldNotes() const noexcept { return heldCount > 0; }

    // Returns the pitch-class mask to resonate with, honouring Freeze Last Chord.
    std::uint16_t getActiveMask (bool freezeLastChord) const noexcept
    {
        if (heldCount > 0)
            return liveMask;

        return freezeLastChord ? frozenMask : 0;
    }

    int getLowestHeldNote() const noexcept { return lowestHeldNote; }

private:
    void rebuildLiveMask() noexcept
    {
        liveMask = 0;
        lowestHeldNote = -1;
        for (int n = 0; n < 128; ++n)
        {
            if (held[(size_t) n])
            {
                liveMask |= static_cast<std::uint16_t> (1u << (n % 12));
                if (lowestHeldNote < 0)
                    lowestHeldNote = n;
            }
        }
    }

    std::array<bool, 128> held {};
    int heldCount = 0;
    int lowestHeldNote = -1;
    std::uint16_t liveMask = 0;
    std::uint16_t frozenMask = 0;
};
} // namespace nscm
