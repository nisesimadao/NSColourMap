#pragma once

#include <array>
#include <algorithm>
#include <cmath>

#include "ResonatorBank.h"
#include "ColourProcessor.h"
#include "CharacterModes.h"
#include "TargetNoteGenerator.h"

namespace nscm
{
inline float clampf (float v, float lo, float hi) noexcept { return v < lo ? lo : (v > hi ? hi : v); }

// The audible heart of NSColourMap. It tunes the active band to the pitch grid
// with a resonator layer, then **energy-matches the tuned signal to the input**
// so the colour is always as loud and present as the dry — this is what makes
// the effect obvious (see memory: the previous resonator-only build was
// inaudible). COLOR 0-100% blends dry->tuned; COLOR 100-200% adds resonance Q,
// harmonic drive and a colourful decay tail (spec v0.5 §9.1 / §12.2).
class ColourMappingCore
{
public:
    void prepare (double sr) noexcept
    {
        sampleRate = (float) (sr > 0.0 ? sr : 44100.0);
        bank.prepare (sampleRate, 0);
        colour.prepare (sampleRate);
        // Envelope follower for energy matching (~12 ms).
        envCoeff  = std::exp (-1.0f / (0.012f * sampleRate));
        gateCoeff = std::exp (-1.0f / (0.040f * sampleRate));
        reset();
    }

    void reset() noexcept
    {
        bank.reset();
        colour.reset();
        inEnv.fill (0.0f);
        wetEnv.fill (0.0f);
        matchGain.fill (1.0f);
        gateGain.fill (1.0f);
    }

    void setTargets (const TargetNoteList& list, int maxVoices) noexcept
    {
        bank.setTargets (list, maxVoices);
    }

    int getActiveVoiceCount() const noexcept { return bank.getActiveCount(); }

    struct Settings
    {
        float color01    = 0.65f; // min(COLOR, 1)
        float colorBoost = 0.0f;  // max(COLOR - 1, 0)
        float amount     = 0.70f;
        float gate       = 0.0f;
        CharacterProfile profile {};
    };

    struct Energies { float input = 0.0f, tuned = 0.0f, colored = 0.0f; };

    // 'tuned' starts as a copy of 'dryActive'; it is overwritten with the
    // processed (coloured) active band. Audio-thread safe, no allocation.
    void process (float* const* tuned, const float* const* dryActive,
                  int numChannels, int numSamples, const Settings& s, Energies& e) noexcept
    {
        const auto& prof = s.profile;

        // ── Resonator layer (pitch-grid tuned) ─────────────────────────────────
        ResonatorBank::Tuning t;
        t.q           = prof.qBase + s.color01 * prof.qColor + s.colorBoost * prof.tailQ;
        t.drive       = prof.drive * 0.5f;
        t.stereoCents = 4.0f + prof.width * 8.0f;
        t.baseGain    = 1.0f;
        const float glideSec = 0.030f * prof.glideScale;
        t.glideCoeff  = glideSec <= 0.0f ? 0.0f
                                         : std::exp (-((float) numSamples / sampleRate) / glideSec);
        bank.process (tuned, numChannels, numSamples, t);

        // ── Colour shaping (drive / high emphasis / width) ─────────────────────
        ColourProcessor::Settings cs;
        cs.colour   = s.color01 + s.colorBoost * 0.5f;
        cs.drive    = prof.drive * (0.5f + s.colorBoost);
        cs.width    = prof.width;
        cs.highEmph = prof.highEmph;
        colour.process (tuned, numChannels, numSamples, cs);

        // ── Energy match + blend + gate ───────────────────────────────────────
        const float intensity = clampf (s.amount * s.color01 * prof.colorResponse, 0.0f, 1.0f);
        const float boostAdd  = s.colorBoost * 0.6f;
        const float gateDepth = s.gate;

        double inAcc = 0.0, tunedAcc = 0.0, colAcc = 0.0;

        for (int ch = 0; ch < numChannels && ch < 2; ++ch)
        {
            float* w        = tuned[(size_t) ch];
            const float* d  = dryActive[(size_t) ch];
            float ie = inEnv[(size_t) ch];
            float we = wetEnv[(size_t) ch];
            float mg = matchGain[(size_t) ch];
            float gg = gateGain[(size_t) ch];

            for (int n = 0; n < numSamples; ++n)
            {
                const float dry = d[n];
                const float ad  = std::abs (dry);
                const float aw  = std::abs (w[n]);

                ie = ad + envCoeff * (ie - ad);
                we = aw + envCoeff * (we - aw);

                // Match the tuned level to the input level.
                const float targetGain = clampf (ie / (we + 1.0e-4f), 0.0f, 12.0f);
                mg += 0.05f * (targetGain - mg);

                // Gate: duck sustained tail when the input has gone quiet.
                const float gTarget = (ie > 0.0025f) ? 1.0f : (1.0f - gateDepth);
                gg = gTarget + gateCoeff * (gg - gTarget);

                const float tunedMatched = w[n] * mg * gg;

                // Blend dry active -> tuned, plus extra resonance tail for COLOR>100.
                float processed = dry + intensity * (tunedMatched - dry) + boostAdd * tunedMatched;

                w[n] = processed;

                inAcc    += (double) dry * dry;
                tunedAcc += (double) (tunedMatched - dry) * (tunedMatched - dry);
                colAcc   += (double) (boostAdd * tunedMatched) * (boostAdd * tunedMatched);
            }

            inEnv[(size_t) ch]    = ie;
            wetEnv[(size_t) ch]   = we;
            matchGain[(size_t) ch]= mg;
            gateGain[(size_t) ch] = gg;
        }

        const int chCount = numChannels < 2 ? numChannels : 2;
        const float inv = 1.0f / (float) std::max (1, numSamples * chCount);
        e.input   = std::sqrt ((float) inAcc    * inv);
        e.tuned   = std::sqrt ((float) tunedAcc * inv);
        e.colored = std::sqrt ((float) colAcc   * inv);
    }

private:
    ResonatorBank   bank;
    ColourProcessor colour;
    float sampleRate = 44100.0f;
    float envCoeff = 0.0f, gateCoeff = 0.0f;
    std::array<float, 2> inEnv {}, wetEnv {}, matchGain {}, gateGain {};
};
} // namespace nscm
