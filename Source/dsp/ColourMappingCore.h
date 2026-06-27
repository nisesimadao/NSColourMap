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

// The audible heart of NSColourMap. Two layers create the pitch-grid colour:
//   1. A grid OSCILLATOR bank — sine+harmonic oscillators AT the grid frequencies,
//      driven by the input's amplitude envelope. This SYNTHESISES in-key tones from
//      any source (noise, off-key bass, chops), so the colour is obvious and truly
//      tonal — a resonator bank alone can only emphasise frequencies already in the
//      input and fails on off-grid material.
//   2. A resonator bank — emphasises existing energy near the grid for texture.
// The combined signal is energy-matched to the input so it is always present, then
// blended dry->tuned by COLOR/Amount. COLOR>100% adds drive, resonator tail and
// extra oscillator harmonics (spec v0.5 §9.1 / §12.2).
class ColourMappingCore
{
public:
    void prepare (double sr) noexcept
    {
        sampleRate = (float) (sr > 0.0 ? sr : 44100.0);
        bank.prepare (sampleRate, 0);
        colour.prepare (sampleRate);
        envCoeff  = std::exp (-1.0f / (0.008f * sampleRate)); // ~8 ms drive envelope
        matchCoef = std::exp (-1.0f / (0.012f * sampleRate));
        gateCoeff = std::exp (-1.0f / (0.040f * sampleRate));
        reset();
    }

    void reset() noexcept
    {
        bank.reset();
        colour.reset();
        phase.fill (0.0f);
        driveEnv = 0.0f;
        inEnv.fill (0.0f); wetEnv.fill (0.0f); matchGain.fill (1.0f); gateGain.fill (1.0f);
    }

    void setTargets (const TargetNoteList& list, int maxVoices) noexcept
    {
        maxVoices = maxVoices < 1 ? 1 : (maxVoices > kMaxVoices ? kMaxVoices : maxVoices);
        // Detect a brand-new note set so we can reset oscillator phases cleanly.
        targets = list;
        if (targets.count > maxVoices) targets.count = maxVoices;
        bank.setTargets (list, maxVoices);
    }

    int getActiveVoiceCount() const noexcept { return targets.count; }

    struct Settings
    {
        float color01    = 0.65f;
        float colorBoost = 0.0f;
        float amount     = 0.70f;
        float gate       = 0.0f;
        CharacterProfile profile {};
    };

    struct Energies { float input = 0.0f, tuned = 0.0f, colored = 0.0f; };

    void process (float* const* tuned, const float* const* dryActive,
                  int numChannels, int numSamples, const Settings& s, Energies& e) noexcept
    {
        const auto& prof = s.profile;
        const int chN = numChannels < 2 ? numChannels : 2;

        // ── Layer 2: resonator emphasis (tuned currently holds a copy of dryActive)
        ResonatorBank::Tuning t;
        t.q           = prof.qBase + s.color01 * prof.qColor + s.colorBoost * prof.tailQ;
        t.drive       = prof.drive;
        t.stereoCents = 4.0f + prof.width * 8.0f;
        t.baseGain    = 1.0f;
        const float glideSec = 0.030f * prof.glideScale;
        t.glideCoeff  = glideSec <= 0.0f ? 0.0f : std::exp (-((float) numSamples / sampleRate) / glideSec);
        bank.process (tuned, numChannels, numSamples, t);

        // ── Layer 1: grid oscillators driven by the input envelope ─────────────
        const int   nv     = targets.count;
        const float oscNorm = nv > 0 ? 1.0f / std::sqrt ((float) nv) : 0.0f;
        const float harm2  = 0.35f + 0.25f * s.colorBoost;   // 2nd harmonic
        const float harm3  = 0.15f * s.color01 + 0.2f * s.colorBoost; // 3rd harmonic
        const float twoPi  = 6.28318530717958648f;
        const float resMix = 0.22f;                           // small resonator emphasis (texture)
        const float oscMix = 1.15f + 0.45f * s.colorBoost;    // oscillator grid weight (dominant)

        std::array<float, kMaxVoices> inc {};
        for (int v = 0; v < nv; ++v)
            inc[(size_t) v] = twoPi * targets.freqs[(size_t) v] / sampleRate;

        for (int n = 0; n < numSamples; ++n)
        {
            // mono input drive envelope
            float mono = 0.0f;
            for (int ch = 0; ch < chN; ++ch) mono += dryActive[(size_t) ch][n];
            mono = std::abs (mono / (float) (chN > 0 ? chN : 1));
            driveEnv = mono + envCoeff * (driveEnv - mono);

            // sum oscillators
            float osc = 0.0f;
            for (int v = 0; v < nv; ++v)
            {
                float p = phase[(size_t) v] + inc[(size_t) v];
                if (p >= twoPi) p -= twoPi;
                phase[(size_t) v] = p;
                const float s1 = std::sin (p);
                osc += s1 + harm2 * std::sin (2.0f * p) + harm3 * std::sin (3.0f * p);
            }
            osc *= oscNorm * driveEnv;

            // combine oscillator grid (dominant) with resonator emphasis
            for (int ch = 0; ch < chN; ++ch)
                tuned[(size_t) ch][n] = oscMix * osc + resMix * tuned[(size_t) ch][n];
        }

        // ── Colour shaping ─────────────────────────────────────────────────────
        ColourProcessor::Settings cs;
        cs.colour   = s.color01 + s.colorBoost * 0.5f;
        cs.drive    = prof.drive * 0.4f;
        cs.width    = prof.width;
        cs.highEmph = prof.highEmph;
        colour.process (tuned, numChannels, numSamples, cs);

        // ── Energy match + blend + gate ───────────────────────────────────────
        const float intensity = clampf (s.color01 * prof.colorResponse * (0.55f + 0.45f * s.amount), 0.0f, 1.0f);
        const float boostAdd  = s.colorBoost * 0.6f;
        const float gateDepth = s.gate;

        double inAcc = 0.0, tunedAcc = 0.0, colAcc = 0.0;

        for (int ch = 0; ch < chN; ++ch)
        {
            float* w        = tuned[(size_t) ch];
            const float* d  = dryActive[(size_t) ch];
            float ie = inEnv[(size_t) ch], we = wetEnv[(size_t) ch], mg = matchGain[(size_t) ch], gg = gateGain[(size_t) ch];

            for (int n = 0; n < numSamples; ++n)
            {
                const float dry = d[n];
                const float ad = std::abs (dry), aw = std::abs (w[n]);
                ie = ad + matchCoef * (ie - ad);
                we = aw + matchCoef * (we - aw);

                const float targetGain = clampf (1.3f * ie / (we + 1.0e-4f), 0.0f, 16.0f);
                mg += 0.05f * (targetGain - mg);

                const float gTarget = (ie > 0.0025f) ? 1.0f : (1.0f - gateDepth);
                gg = gTarget + gateCoeff * (gg - gTarget);

                const float tunedMatched = w[n] * mg * gg;
                const float processed = dry + intensity * (tunedMatched - dry) + boostAdd * tunedMatched;
                w[n] = processed;

                inAcc    += (double) dry * dry;
                tunedAcc += (double) (tunedMatched - dry) * (tunedMatched - dry);
                colAcc   += (double) (boostAdd * tunedMatched) * (boostAdd * tunedMatched);
            }

            inEnv[(size_t) ch] = ie; wetEnv[(size_t) ch] = we; matchGain[(size_t) ch] = mg; gateGain[(size_t) ch] = gg;
        }

        const float inv = 1.0f / (float) std::max (1, numSamples * std::max (1, chN));
        e.input   = std::sqrt ((float) inAcc * inv);
        e.tuned   = std::sqrt ((float) tunedAcc * inv);
        e.colored = std::sqrt ((float) colAcc * inv);
    }

private:
    ResonatorBank   bank;
    ColourProcessor colour;
    TargetNoteList  targets;
    float sampleRate = 44100.0f;
    float envCoeff = 0.0f, matchCoef = 0.0f, gateCoeff = 0.0f;
    float driveEnv = 0.0f;
    std::array<float, kMaxVoices> phase {};
    std::array<float, 2> inEnv {}, wetEnv {}, matchGain {}, gateGain {};
};
} // namespace nscm
