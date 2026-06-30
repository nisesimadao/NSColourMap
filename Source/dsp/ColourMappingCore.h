#pragma once

#include <cstddef>

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
        fastCoef  = std::exp (-1.0f / (0.002f * sampleRate)); // ~2 ms (morph contour)
        slowCoef  = std::exp (-1.0f / (0.030f * sampleRate)); // ~30 ms (makeup)
        reset();
    }

    void reset() noexcept
    {
        bank.reset();
        colour.reset();
        phase.fill (0.0f);
        shimPhase.fill (0.0f);
        melodyEnv.fill (0.0f);
        for (auto& d : melodyDet)
            d.reset();
        mapPhase.fill (0.0f);
        mapEnv.fill (0.0f);
        mapBodyEnv.fill (0.0f);
        for (auto& d : mapDet)
            d.reset();
        lfoPhase = 0.0f;
        driveEnv = 0.0f;
        inEnv.fill (0.0f); wetEnv.fill (0.0f); matchGain.fill (1.0f); gateGain.fill (1.0f);
        fastInEnv.fill (0.0f); procEnv.fill (0.0f);
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
        float melody     = 0.0f;  // 0 = existing wide grid colour, 1 = foreground the most input-related grid notes
        float gate       = 0.0f;
        float morph      = 0.0f; // imprint the dry's fast contour onto the wet (transient preserve / tail control)
        float sizzle     = 0.0f; // user "Air" — smooth high-frequency crispness
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
        t.air         = clampf (prof.air * (0.4f + 0.6f * s.color01) + s.colorBoost * 0.4f, 0.0f, 1.2f);
        const float glideSec = 0.030f * prof.glideScale;
        t.glideCoeff  = glideSec <= 0.0f ? 0.0f : std::exp (-((float) numSamples / sampleRate) / glideSec);
        bank.process (tuned, numChannels, numSamples, t);

        // ── Layer 1: grid oscillators driven by the input envelope ─────────────
        const int   nv     = targets.count;
        const float userMelody = clampf (s.melody, 0.0f, 1.0f);
        const float mapFocus   = clampf (userMelody + prof.mapFocus * (1.0f - userMelody), 0.0f, 1.0f);
        const float mapPurity  = clampf (prof.mapPurity * mapFocus, 0.0f, 1.0f);
        const float harm2  = (0.35f + 0.25f * s.colorBoost) * (1.0f - 0.30f * mapFocus) * (1.0f - 0.28f * mapPurity);   // 2nd harmonic
        const float harm3  = (0.15f * s.color01 + 0.2f * s.colorBoost) * (1.0f - 0.45f * mapFocus) * (1.0f - 0.50f * mapPurity); // 3rd harmonic
        const float twoPi  = 6.28318530717958648f;
        const float nyq    = 0.45f * sampleRate;
        const float resMix = 0.22f;                           // small resonator emphasis (texture)
        const float oscMix = 1.15f + 0.45f * s.colorBoost;    // oscillator grid weight (dominant)
        // Shimmer: a detuned octave-up partial that slowly beats -> living sparkle.
        const float shimAmt = clampf ((prof.shimmer * (0.35f + 0.65f * s.color01) + s.colorBoost * 0.4f)
                                      * (1.0f - 0.25f * mapFocus) * (1.0f - 0.45f * mapPurity), 0.0f, 1.3f);
        const float lfoInc  = twoPi * 0.6f / sampleRate;      // ~0.6 Hz movement
        const float melCoef = std::exp (-1.0f / (0.055f * sampleRate));
        const float detQ    = 7.0f - 4.0f * mapFocus;         // broader in Map mode: off-grid notes pull to nearest slot
        const float mapLayer = clampf (prof.mapFocus * (0.75f + 0.25f * s.color01)
                                       * (0.75f + 0.25f * s.amount), 0.0f, 1.0f);
        const float mapAttackCoeff = std::exp (-1.0f / (0.0045f * sampleRate));
        const float mapReleaseCoeff = std::exp (-1.0f / (0.060f * sampleRate));
        const float mapBodyCoeff = std::exp (-1.0f / (0.090f * sampleRate));
        const float mapSecond = 0.16f + 0.11f * s.color01 + 0.08f * s.colorBoost;
        const float mapThird = 0.055f + 0.050f * s.color01 + 0.045f * s.colorBoost;
        const float mapEdge = clampf (0.06f + 0.10f * s.colorBoost + 0.05f * (1.0f - mapPurity), 0.0f, 0.18f);

        std::array<float, kMaxVoices> inc {};
        std::array<bool,  kMaxVoices> use2 {}, use3 {}, useShim {};
        for (int v = 0; v < nv; ++v)
        {
            const float f = targets.freqs[(std::size_t) v];
            inc[(std::size_t) v]     = twoPi * f / sampleRate;
            use2[(std::size_t) v]    = (2.0f * f) < nyq;           // band-limit harmonics
            use3[(std::size_t) v]    = (3.0f * f) < nyq;
            useShim[(std::size_t) v] = (2.0f * f) < nyq;
            melodyDet[(std::size_t) v].setCoeffs (f, detQ, sampleRate);
        }
        for (int v = nv; v < kMaxVoices; ++v)
            melodyEnv[(std::size_t) v] = 0.0f;

        int mapSlotCount = 0;
        if (mapLayer > 0.0f && nv > 0)
        {
            constexpr int lowNote = 0;
            constexpr int highNote = 127;
            for (int note = lowNote; note <= highNote && mapSlotCount < kMapSlots; ++note)
            {
                int bestNote = targets.notes[0];
                int bestDist = 999;
                for (int v = 0; v < nv; ++v)
                {
                    const int tn = targets.notes[(std::size_t) v];
                    const int dist = std::abs (tn - note);
                    if (dist < bestDist)
                    {
                        bestDist = dist;
                        bestNote = tn;
                    }
                }

                const float srcHz = midiNoteToHz ((float) note);
                const float dstHz = midiNoteToHz ((float) bestNote);
                if (srcHz < 20.0f || srcHz > nyq || dstHz > nyq)
                    continue;

                const auto i = (std::size_t) mapSlotCount++;
                mapDet[i].setCoeffs (srcHz, 8.0f, sampleRate);
                mapInc[i] = twoPi * dstHz / sampleRate;
                mapDstHz[i] = dstHz;
                mapShiftSemis[i] = (float) (bestNote - note);
            }
        }
        for (int i = mapSlotCount; i < kMapSlots; ++i)
        {
            mapEnv[(std::size_t) i] = 0.0f;
            mapBodyEnv[(std::size_t) i] = 0.0f;
        }

        for (int n = 0; n < numSamples; ++n)
        {
            // mono input drive envelope
            float monoRaw = 0.0f;
            for (int ch = 0; ch < chN; ++ch) monoRaw += dryActive[(std::size_t) ch][n];
            monoRaw /= (float) (chN > 0 ? chN : 1);
            const float mono = std::abs (monoRaw);
            driveEnv = mono + envCoeff * (driveEnv - mono);

            float melMax = 0.0f;
            if (mapFocus > 0.0f)
            {
                for (int v = 0; v < nv; ++v)
                {
                    const auto i = (std::size_t) v;
                    const float det = std::abs (melodyDet[i].processBandpass (monoRaw));
                    melodyEnv[i] = det + melCoef * (melodyEnv[i] - det);
                    if (melodyEnv[i] > melMax) melMax = melodyEnv[i];
                }
            }

            float mapOsc = 0.0f;
            float mapPower = 0.0f;
            if (mapSlotCount > 0)
            {
                for (int i = 0; i < mapSlotCount; ++i)
                {
                    const auto idx = (std::size_t) i;
                    const float bp = mapDet[idx].processBandpass (monoRaw);
                    const float a = std::abs (bp);
                    const float envCoeffForSample = a > mapEnv[idx] ? mapAttackCoeff : mapReleaseCoeff;
                    mapEnv[idx] = a + envCoeffForSample * (mapEnv[idx] - a);
                    mapBodyEnv[idx] = a + mapBodyCoeff * (mapBodyEnv[idx] - a);

                    float p = mapPhase[idx] + mapInc[idx];
                    if (p >= twoPi) p -= twoPi;
                    mapPhase[idx] = p;

                    const float shift = std::abs (mapShiftSemis[idx]);
                    const float shifted = shift > 0.5f ? 1.0f : 0.45f;
                    const float wild = 1.0f + 0.08f * mapPurity * shift;
                    const float body = mapBodyEnv[idx] + 1.0e-5f;
                    const float contour = clampf (mapEnv[idx] / body, 0.70f, 1.85f);
                    const float sourceShape = clampf (bp / body, -1.0f, 1.0f);
                    const float shapedP = p + 0.020f * mapPurity * sourceShape;
                    float y = std::sin (shapedP);
                    if (2.0f * mapDstHz[idx] < nyq)
                        y += mapSecond * std::sin (2.0f * shapedP + 0.45f + 0.12f * sourceShape);
                    if (3.0f * mapDstHz[idx] < nyq)
                        y += mapThird * std::sin (3.0f * shapedP + 1.05f);
                    y += mapEdge * std::tanh (2.2f * sourceShape) * std::sin (shapedP + 1.2f);
                    const float w = mapEnv[idx] * contour * shifted * wild;
                    mapOsc += w * y;
                    mapPower += w * w;
                }
                if (mapPower > 1.0e-8f)
                    mapOsc *= 1.0f / std::sqrt (mapPower * 6.8f);
                mapOsc *= driveEnv;
            }

            lfoPhase += lfoInc;
            if (lfoPhase >= twoPi) lfoPhase -= twoPi;
            const float shimDetune = 1.0f + 0.004f * std::sin (lfoPhase); // slow octave detune

            // sum oscillators (fundamental + harmonics + detuned shimmer octave)
            float osc = 0.0f, shim = 0.0f;
            float weightPower = 0.0f;
            for (int v = 0; v < nv; ++v)
            {
                const auto i = (std::size_t) v;
                float voiceWeight = 1.0f;
                if (mapFocus > 0.0f && melMax > 1.0e-6f)
                {
                    const float rel = clampf (melodyEnv[i] / (melMax + 1.0e-6f), 0.0f, 1.0f);
                    const float tightRel = rel * rel * ((1.0f - mapPurity) + mapPurity * rel);
                    const float focused = (0.18f - 0.10f * mapPurity) + (1.85f + 0.95f * mapPurity) * tightRel;
                    voiceWeight = (1.0f - mapFocus) + mapFocus * focused;
                }

                float p = phase[(std::size_t) v] + inc[(std::size_t) v];
                if (p >= twoPi) p -= twoPi;
                phase[(std::size_t) v] = p;
                osc += voiceWeight * (std::sin (p)
                     + (use2[(std::size_t) v] ? harm2 * std::sin (2.0f * p) : 0.0f)
                     + (use3[(std::size_t) v] ? harm3 * std::sin (3.0f * p) : 0.0f));
                weightPower += voiceWeight * voiceWeight;

                if (shimAmt > 0.0f && useShim[(std::size_t) v])
                {
                    float sp = shimPhase[(std::size_t) v] + inc[(std::size_t) v] * 2.0f * shimDetune;
                    if (sp >= twoPi) sp -= twoPi;
                    shimPhase[(std::size_t) v] = sp;
                    shim += voiceWeight * std::sin (sp);
                }
            }
            const float oscNorm = weightPower > 1.0e-6f ? 1.0f / std::sqrt (weightPower) : 0.0f;
            osc = (osc + shimAmt * shim) * oscNorm * driveEnv;
            if (mapLayer > 0.0f)
                osc = (1.0f - 0.50f * mapLayer) * osc + (1.12f + 0.38f * s.colorBoost) * mapLayer * mapOsc;

            // combine oscillator grid (dominant) with resonator emphasis
            for (int ch = 0; ch < chN; ++ch)
                tuned[(std::size_t) ch][n] = oscMix * osc + resMix * tuned[(std::size_t) ch][n];
        }

        // ── Colour shaping ─────────────────────────────────────────────────────
        ColourProcessor::Settings cs;
        cs.colour   = s.color01 + s.colorBoost * 0.5f;
        cs.drive    = prof.drive * 0.4f;
        cs.air      = clampf (prof.air * (0.3f + 0.7f * s.color01) + s.colorBoost * 0.5f, 0.0f, 1.2f);
        cs.sizzle   = s.sizzle;
        cs.width    = prof.width;
        cs.highEmph = prof.highEmph;
        colour.process (tuned, numChannels, numSamples, cs);

        // ── Energy match + morph + blend + gate + loudness makeup ─────────────
        const float intensity = clampf (s.color01 * prof.colorResponse * (0.55f + 0.45f * s.amount), 0.0f, 1.0f);
        const float boostAdd  = s.colorBoost * 0.6f;
        const float gateDepth = s.gate;
        const float morph     = s.morph;

        double inAcc = 0.0, tunedAcc = 0.0, colAcc = 0.0;

        for (int ch = 0; ch < chN; ++ch)
        {
            float* w        = tuned[(std::size_t) ch];
            const float* d  = dryActive[(std::size_t) ch];
            float ie = inEnv[(std::size_t) ch], we = wetEnv[(std::size_t) ch], mg = matchGain[(std::size_t) ch], gg = gateGain[(std::size_t) ch];
            float fie = fastInEnv[(std::size_t) ch], pe = procEnv[(std::size_t) ch];

            for (int n = 0; n < numSamples; ++n)
            {
                const float dry = d[n];
                const float ad = std::abs (dry), aw = std::abs (w[n]);
                ie  = ad + matchCoef * (ie - ad);
                we  = aw + matchCoef * (we - aw);
                fie = ad + fastCoef  * (fie - ad);      // fast dry envelope (~2 ms)

                const float targetGain = clampf (1.3f * ie / (we + 1.0e-4f), 0.0f, 16.0f);
                mg += 0.05f * (targetGain - mg);

                const float gTarget = (ie > 0.0025f) ? 1.0f : (1.0f - gateDepth);
                gg = gTarget + gateCoeff * (gg - gTarget);

                float tunedMatched = w[n] * mg * gg;

                // Morph: imprint the dry's fast contour (transient preserve + tail control).
                if (morph > 0.0f)
                {
                    const float contour = clampf (fie / (ie + 1.0e-4f), 0.0f, 3.0f);
                    tunedMatched *= (1.0f - morph) + morph * contour;
                }

                float processed = dry + intensity * (tunedMatched - dry) + boostAdd * tunedMatched;

                // Loudness makeup: keep the colour from exceeding ~1.2x the input
                // level so pushing COLOR never clips (Chroma-style clean staging).
                pe = std::abs (processed) + slowCoef * (pe - std::abs (processed));
                const float ceiling = 1.2f * ie + 1.0e-4f;
                if (pe > ceiling)
                    processed *= ceiling / pe;

                w[n] = processed;

                inAcc    += (double) dry * dry;
                tunedAcc += (double) (tunedMatched - dry) * (tunedMatched - dry);
                colAcc   += (double) (boostAdd * tunedMatched) * (boostAdd * tunedMatched);
            }

            inEnv[(std::size_t) ch] = ie; wetEnv[(std::size_t) ch] = we; matchGain[(std::size_t) ch] = mg;
            gateGain[(std::size_t) ch] = gg; fastInEnv[(std::size_t) ch] = fie; procEnv[(std::size_t) ch] = pe;
        }

        const float inv = 1.0f / (float) std::max (1, numSamples * std::max (1, chN));
        e.input   = std::sqrt ((float) inAcc * inv);
        e.tuned   = std::sqrt ((float) tunedAcc * inv);
        e.colored = std::sqrt ((float) colAcc * inv);
    }

private:
    static constexpr int kMapSlots = 128;

    ResonatorBank   bank;
    ColourProcessor colour;
    TargetNoteList  targets;
    float sampleRate = 44100.0f;
    float envCoeff = 0.0f, matchCoef = 0.0f, gateCoeff = 0.0f, fastCoef = 0.0f, slowCoef = 0.0f;
    float driveEnv = 0.0f;
    float lfoPhase = 0.0f;
    std::array<float, kMaxVoices> phase {}, shimPhase {};
    std::array<SvfResonator, kMaxVoices> melodyDet {};
    std::array<float, kMaxVoices> melodyEnv {};
    std::array<SvfResonator, kMapSlots> mapDet {};
    std::array<float, kMapSlots> mapPhase {}, mapInc {}, mapEnv {}, mapBodyEnv {}, mapDstHz {}, mapShiftSemis {};
    std::array<float, 2> inEnv {}, wetEnv {}, matchGain {}, gateGain {}, fastInEnv {}, procEnv {};
};
} // namespace nscm
