// Header-only smoke tests for the music-theory / target-generation DSP.
// These classes carry no JUCE dependency so the test builds in seconds.

#include "dsp/ScaleNoteSet.h"
#include "dsp/MidiChordState.h"
#include "dsp/TargetNoteGenerator.h"
#include "dsp/ColourMappingCore.h"
#include "dsp/CharacterModes.h"

#include <cstdio>
#include <cmath>
#include <vector>
#include <random>

static int failures = 0;

// RMS energy in a narrow band around 'freq', measured with a stable SVF bandpass
// detector (Goertzel is numerically unreliable on long broadband signals).
static double bandRms (const std::vector<float>& x, double freq, double sr)
{
    nscm::SvfResonator det;
    det.setCoeffs ((float) freq, 20.0f, (float) sr);
    double acc = 0.0;
    for (float v : x) { const float bp = det.processBandpass (v); acc += (double) bp * bp; }
    return std::sqrt (acc / (double) x.size());
}

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

    // ── ColourMappingCore audibility ──────────────────────────────────────────
    // The previous build was inaudible; the core must (a) keep the wet as loud as
    // the input and (b) concentrate broadband energy onto the pitch grid.
    {
        const double sr = 48000.0;
        const int    N  = 24000;
        ColourMappingCore core;
        core.prepare (sr);

        TargetNoteList list;            // single grid note: A4 = 440 Hz
        list.count = 1; list.notes[0] = 69; list.freqs[0] = 440.0f;
        core.setTargets (list, 32);

        std::mt19937 rng (1);
        std::uniform_real_distribution<float> dist (-0.3f, 0.3f);
        std::vector<float> inL ((size_t) N), inR ((size_t) N), outL ((size_t) N), outR ((size_t) N);
        for (int i = 0; i < N; ++i) { inL[(size_t) i] = dist (rng); inR[(size_t) i] = dist (rng); }

        ColourMappingCore::Settings s;
        s.color01 = 1.0f; s.colorBoost = 0.0f; s.amount = 1.0f; s.gate = 0.0f;
        s.profile = getCharacterProfile (Character::color);

        const int B = 256;
        std::vector<float> tL ((size_t) B), tR ((size_t) B), dL ((size_t) B), dR ((size_t) B);
        for (int off = 0; off < N; off += B)
        {
            const int n = std::min (B, N - off);
            for (int i = 0; i < n; ++i)
            {
                tL[(size_t) i] = dL[(size_t) i] = inL[(size_t) (off + i)];
                tR[(size_t) i] = dR[(size_t) i] = inR[(size_t) (off + i)];
            }
            float* tuned[2]   = { tL.data(), tR.data() };
            const float* dry[2] = { dL.data(), dR.data() };
            ColourMappingCore::Energies e;
            core.process (tuned, dry, 2, n, s, e);
            for (int i = 0; i < n; ++i) { outL[(size_t) (off + i)] = tL[(size_t) i]; outR[(size_t) (off + i)] = tR[(size_t) i]; }
        }

        // Analyse the settled second half.
        std::vector<float> inHalf (inL.begin() + N / 2, inL.end());
        std::vector<float> outHalf (outL.begin() + N / 2, outL.end());
        auto rms = [] (const std::vector<float>& v) { double a = 0; for (float x : v) a += (double) x * x; return std::sqrt (a / (double) v.size()); };

        const double inRms  = rms (inHalf);
        const double outRms = rms (outHalf);
        const double e440   = bandRms (outHalf, 440.0, sr);
        const double e1000  = bandRms (outHalf, 1000.0, sr);
        const double e440In = bandRms (inHalf, 440.0, sr);

        CHECK (outRms > 0.3 * inRms);   // wet is present / audible
        CHECK (e440 > 4.0 * e1000);     // energy concentrated on the grid note
        CHECK (e440 > 3.0 * e440In);    // grid note emphasised vs raw input
    }

    // ── Melody focus knob ─────────────────────────────────────────────────────
    // At zero it preserves the wide grid character; when raised it should make a
    // grid note related to the input stand out more than unrelated grid notes.
    {
        const double sr = 48000.0;
        const int    N  = 24000;

        TargetNoteList list;
        list.count = 3;
        list.notes[0] = 60; list.freqs[0] = midiNoteToHz (60.0f); // C4
        list.notes[1] = 67; list.freqs[1] = midiNoteToHz (67.0f); // G4, input-related
        list.notes[2] = 70; list.freqs[2] = midiNoteToHz (70.0f); // Bb4

        auto run = [&] (float melody) {
            ColourMappingCore core;
            core.prepare (sr);
            core.setTargets (list, 32);

            std::vector<float> inL ((size_t) N), inR ((size_t) N), outL ((size_t) N), outR ((size_t) N);
            for (int i = 0; i < N; ++i)
            {
                const float t = (float) i / (float) sr;
                const float v = 0.24f * std::sin (2.0f * 3.14159265358979323846f * list.freqs[1] * t)
                              + 0.08f * std::sin (2.0f * 3.14159265358979323846f * 1234.0f * t);
                inL[(size_t) i] = v;
                inR[(size_t) i] = v;
            }

            ColourMappingCore::Settings s;
            s.color01 = 1.0f; s.amount = 1.0f; s.melody = melody;
            s.profile = getCharacterProfile (Character::color);

            const int B = 256;
            std::vector<float> tL ((size_t) B), tR ((size_t) B), dL ((size_t) B), dR ((size_t) B);
            for (int off = 0; off < N; off += B)
            {
                const int n = std::min (B, N - off);
                for (int i = 0; i < n; ++i)
                {
                    tL[(size_t) i] = dL[(size_t) i] = inL[(size_t) (off + i)];
                    tR[(size_t) i] = dR[(size_t) i] = inR[(size_t) (off + i)];
                }
                float* tuned[2] = { tL.data(), tR.data() };
                const float* dry[2] = { dL.data(), dR.data() };
                ColourMappingCore::Energies e;
                core.process (tuned, dry, 2, n, s, e);
                for (int i = 0; i < n; ++i) outL[(size_t) (off + i)] = tL[(size_t) i];
            }

            std::vector<float> outHalf (outL.begin() + N / 2, outL.end());
            const double eG  = bandRms (outHalf, list.freqs[1], sr);
            const double eBb = bandRms (outHalf, list.freqs[2], sr);
            return eG / (eBb + 1.0e-9);
        };

        const double wideRatio    = run (0.0f);
        const double focusedRatio = run (1.0f);
        CHECK (focusedRatio > wideRatio * 1.35);
    }

    // ── Map character pitch-slot remapping ────────────────────────────────────
    // The Map character should move off-grid pitched input toward the nearest
    // grid slot more strongly than the normal Color character.
    {
        const double sr = 48000.0;
        const int    N  = 24000;

        TargetNoteList list;
        list.count = 1;
        list.notes[0] = 60; list.freqs[0] = midiNoteToHz (60.0f); // C4 target

        auto run = [&] (Character ch) {
            ColourMappingCore core;
            core.prepare (sr);
            core.setTargets (list, 32);

            std::vector<float> inL ((size_t) N), inR ((size_t) N), outL ((size_t) N);
            for (int i = 0; i < N; ++i)
            {
                const float t = (float) i / (float) sr;
                const float v = 0.22f * std::sin (2.0f * 3.14159265358979323846f * midiNoteToHz (61.0f) * t);
                inL[(size_t) i] = v;
                inR[(size_t) i] = v;
            }

            ColourMappingCore::Settings s;
            s.color01 = 1.0f; s.amount = 1.0f; s.melody = 0.0f;
            s.profile = getCharacterProfile (ch);

            const int B = 256;
            std::vector<float> tL ((size_t) B), tR ((size_t) B), dL ((size_t) B), dR ((size_t) B);
            for (int off = 0; off < N; off += B)
            {
                const int n = std::min (B, N - off);
                for (int i = 0; i < n; ++i)
                {
                    tL[(size_t) i] = dL[(size_t) i] = inL[(size_t) (off + i)];
                    tR[(size_t) i] = dR[(size_t) i] = inR[(size_t) (off + i)];
                }
                float* tuned[2] = { tL.data(), tR.data() };
                const float* dry[2] = { dL.data(), dR.data() };
                ColourMappingCore::Energies e;
                core.process (tuned, dry, 2, n, s, e);
                for (int i = 0; i < n; ++i) outL[(size_t) (off + i)] = tL[(size_t) i];
            }

            std::vector<float> outHalf (outL.begin() + N / 2, outL.end());
            const double eC  = bandRms (outHalf, midiNoteToHz (60.0f), sr);
            const double eCs = bandRms (outHalf, midiNoteToHz (61.0f), sr);
            return eC / (eCs + 1.0e-9);
        };

        CHECK (run (Character::alien) > run (Character::color) * 1.20);

        auto runMapBody = [&] {
            ColourMappingCore core;
            core.prepare (sr);
            core.setTargets (list, 32);

            std::vector<float> inL ((size_t) N), inR ((size_t) N), outL ((size_t) N);
            for (int i = 0; i < N; ++i)
            {
                const float t = (float) i / (float) sr;
                const float v = 0.22f * std::sin (2.0f * 3.14159265358979323846f * midiNoteToHz (61.0f) * t);
                inL[(size_t) i] = v;
                inR[(size_t) i] = v;
            }

            ColourMappingCore::Settings s;
            s.color01 = 1.0f; s.amount = 1.0f; s.melody = 0.0f;
            s.profile = getCharacterProfile (Character::alien);

            const int B = 256;
            std::vector<float> tL ((size_t) B), tR ((size_t) B), dL ((size_t) B), dR ((size_t) B);
            for (int off = 0; off < N; off += B)
            {
                const int n = std::min (B, N - off);
                for (int i = 0; i < n; ++i)
                {
                    tL[(size_t) i] = dL[(size_t) i] = inL[(size_t) (off + i)];
                    tR[(size_t) i] = dR[(size_t) i] = inR[(size_t) (off + i)];
                }
                float* tuned[2] = { tL.data(), tR.data() };
                const float* dry[2] = { dL.data(), dR.data() };
                ColourMappingCore::Energies e;
                core.process (tuned, dry, 2, n, s, e);
                for (int i = 0; i < n; ++i) outL[(size_t) (off + i)] = tL[(size_t) i];
            }

            std::vector<float> outHalf (outL.begin() + N / 2, outL.end());
            const double eC = bandRms (outHalf, midiNoteToHz (60.0f), sr);
            const double eC2 = bandRms (outHalf, midiNoteToHz (72.0f), sr);
            const double eC3 = bandRms (outHalf, midiNoteToHz (84.0f), sr);
            const double eCs = bandRms (outHalf, midiNoteToHz (61.0f), sr);
            return (eC > eCs * 1.8) && (eC2 > eC * 0.06) && (eC3 > eC * 0.012);
        };

        CHECK (runMapBody());
    }

    if (failures == 0)
        std::printf ("All NSColourMap DSP smoke tests passed.\n");
    else
        std::printf ("%d test(s) failed.\n", failures);

    return failures == 0 ? 0 : 1;
}
