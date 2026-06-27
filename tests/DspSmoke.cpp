// Header-only smoke tests for the music-theory / target-generation DSP.
// These classes carry no JUCE dependency so the test builds in seconds.

#include "dsp/ScaleNoteSet.h"
#include "dsp/MidiChordState.h"
#include "dsp/TargetNoteGenerator.h"

#include <cstdio>
#include <cmath>

static int failures = 0;

#define CHECK(cond)                                                         \
    do {                                                                    \
        if (! (cond)) { std::printf ("FAIL: %s (line %d)\n", #cond, __LINE__); ++failures; } \
    } while (0)

int main()
{
    using namespace nscm;

    // ── ScaleNoteSet ──────────────────────────────────────────────────────────
    {
        // C Major => C D E F G A B
        auto s = makeScaleNoteSet (0, Scale::major);
        CHECK (s.count() == 7);
        CHECK (s.contains (0));   // C
        CHECK (s.contains (4));   // E
        CHECK (! s.contains (1)); // C#

        // C Minor Pentatonic => C Eb F G Bb
        auto mp = makeScaleNoteSet (0, Scale::minorPentatonic);
        CHECK (mp.count() == 5);
        CHECK (mp.contains (0) && mp.contains (3) && mp.contains (5) && mp.contains (7) && mp.contains (10));

        // A Natural Minor == C Major pitch classes (relative minor)
        auto am = makeScaleNoteSet (9, Scale::naturalMinor);
        CHECK (am.mask == s.mask);
    }

    // ── MidiChordState ────────────────────────────────────────────────────────
    {
        MidiChordState chord;
        chord.reset();
        CHECK (! chord.hasHeldNotes());

        chord.noteOn (60); // C4
        chord.noteOn (64); // E4
        chord.noteOn (67); // G4
        CHECK (chord.hasHeldNotes());
        CHECK (chord.getLowestHeldNote() == 60);

        const auto live = chord.getActiveMask (true);
        CHECK ((live & (1u << 0)) && (live & (1u << 4)) && (live & (1u << 7))); // C E G

        chord.noteOff (60);
        chord.noteOff (64);
        chord.noteOff (67);
        CHECK (! chord.hasHeldNotes());

        // Freeze on keeps the last chord; freeze off clears it.
        CHECK (chord.getActiveMask (true) == live);
        CHECK (chord.getActiveMask (false) == 0);
    }

    // ── TargetNoteGenerator ───────────────────────────────────────────────────
    {
        TargetNoteList list;
        auto cmaj = makeScaleNoteSet (0, Scale::major);
        fillTargetNotes (list, cmaj.mask, 0, 32);
        CHECK (list.count > 0 && list.count <= 32);

        // Frequencies must be ascending and positive.
        for (int i = 1; i < list.count; ++i)
            CHECK (list.freqs[(size_t) i] >= list.freqs[(size_t) (i - 1)]);
        CHECK (list.freqs[0] > 0.0f);

        // Empty mask => no targets.
        TargetNoteList empty;
        fillTargetNotes (empty, 0, 0, 32);
        CHECK (empty.count == 0);

        // A440 sanity: MIDI 69 -> ~440 Hz.
        CHECK (std::abs (midiNoteToHz (69.0f) - 440.0f) < 0.01f);
    }

    if (failures == 0)
        std::printf ("All NSColourMap DSP smoke tests passed.\n");
    else
        std::printf ("%d test(s) failed.\n", failures);

    return failures == 0 ? 0 : 1;
}
