#pragma once

#include <array>
#include <cmath>

#include "TargetNoteGenerator.h"

namespace nscm
{
// A single TPT state-variable bandpass resonator (one audio channel of state).
// Reference: Cytomic / Andrew Simper TPT SVF.
struct SvfResonator
{
    float ic1 = 0.0f, ic2 = 0.0f;
    float a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;

    void setCoeffs (float fc, float q, float sampleRate) noexcept
    {
        const float nyq = sampleRate * 0.49f;
        fc = fc < 20.0f ? 20.0f : (fc > nyq ? nyq : fc);
        const float g = std::tan (3.14159265358979323846f * fc / sampleRate);
        const float k = 1.0f / (q < 0.2f ? 0.2f : q);
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    inline float processBandpass (float v0) noexcept
    {
        const float v3 = v0 - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        return v1; // bandpass output
    }

    void reset() noexcept { ic1 = ic2 = 0.0f; }
};

// One resonator voice = stereo pair of bandpass filters tuned to a target note,
// with glide, per-voice gain, Q, drive and a small stereo detune (spec §18.4-18.5).
struct ResonatorVoice
{
    SvfResonator left, right;       // fundamental resonance
    SvfResonator leftHi, rightHi;   // octave-up "air" resonance for shine
    float targetHz  = 0.0f;
    float currentHz = 0.0f;
    bool  active    = false;

    void reset() noexcept
    {
        left.reset();
        right.reset();
        leftHi.reset();
        rightHi.reset();
        currentHz = targetHz;
    }
};

// Bank of resonator voices controlled by a target note list (spec §18).
class ResonatorBank
{
public:
    void prepare (double sr, int /*maxBlock*/) noexcept
    {
        sampleRate = (float) (sr > 0.0 ? sr : 44100.0);
        reset();
    }

    void reset() noexcept
    {
        for (auto& v : voices)
            v.reset();
    }

    // Tuning applied from the active algorithm + macros.
    struct Tuning
    {
        float q          = 8.0f;   // resonator Q
        float drive      = 0.0f;   // 0..1 soft saturation amount
        float stereoCents = 6.0f;  // L/R detune in cents
        float glideCoeff = 0.0f;   // one-pole coeff per block (0 = instant)
        float baseGain   = 1.0f;   // overall wet gain into the bank
        float air        = 0.0f;   // 0..1 octave-up bright resonance (shine)
    };

    // Update the active targets. Called at control rate (per block) — no allocation.
    void setTargets (const TargetNoteList& list, int maxVoices) noexcept
    {
        maxVoices = maxVoices < 1 ? 1 : (maxVoices > kMaxVoices ? kMaxVoices : maxVoices);
        activeCount = list.count < maxVoices ? list.count : maxVoices;

        for (int i = 0; i < kMaxVoices; ++i)
        {
            auto& v = voices[(size_t) i];
            if (i < activeCount)
            {
                v.targetHz = list.freqs[(size_t) i];
                if (! v.active)            // newly activated voice snaps to target
                    v.currentHz = v.targetHz;
                v.active = true;
            }
            else
            {
                v.active = false;
            }
        }
    }

    void process (float* const* channels, int numChannels, int numSamples, const Tuning& t) noexcept
    {
        if (activeCount <= 0)
            return;

        const float norm  = t.baseGain / std::sqrt ((float) activeCount);
        const float detune = std::pow (2.0f, t.stereoCents / 1200.0f);

        // Per-block glide + coefficient update.
        for (int i = 0; i < activeCount; ++i)
        {
            auto& v = voices[(size_t) i];
            v.currentHz = t.glideCoeff <= 0.0f
                              ? v.targetHz
                              : v.targetHz + t.glideCoeff * (v.currentHz - v.targetHz);
            v.left.setCoeffs (v.currentHz / detune, t.q, sampleRate);
            v.right.setCoeffs (v.currentHz * detune, t.q, sampleRate);

            if (t.air > 0.0f)
            {
                // Octave-up resonance, higher Q + a few cents of detune -> shimmer.
                const float hi = v.currentHz * 2.0f;
                v.leftHi.setCoeffs  (hi * 0.9994f, t.q * 1.5f, sampleRate);
                v.rightHi.setCoeffs (hi * 1.0006f, t.q * 1.5f, sampleRate);
            }
        }

        const float driveGain = 1.0f + t.drive * 6.0f;
        const float driveComp = 1.0f / (1.0f + t.drive * 2.0f);
        const bool  useAir    = t.air > 0.0f;
        const float airAmt    = t.air;

        for (int ch = 0; ch < numChannels && ch < 2; ++ch)
        {
            float* data = channels[ch];
            const bool isRight = (ch == 1);

            for (int s = 0; s < numSamples; ++s)
            {
                const float in = data[s];
                float acc = 0.0f;

                for (int i = 0; i < activeCount; ++i)
                {
                    auto& v = voices[(size_t) i];
                    float bp = isRight ? v.right.processBandpass (in)
                                       : v.left.processBandpass (in);
                    if (useAir)
                    {
                        const float hi = isRight ? v.rightHi.processBandpass (in)
                                                 : v.leftHi.processBandpass (in);
                        bp += airAmt * hi;
                    }
                    if (t.drive > 0.0f)
                        bp = std::tanh (bp * driveGain) * driveComp;
                    acc += bp;
                }

                data[s] = acc * norm;
            }
        }
    }

    int getActiveCount() const noexcept { return activeCount; }

    float getVoiceHz (int i) const noexcept
    {
        return (i >= 0 && i < activeCount) ? voices[(size_t) i].currentHz : 0.0f;
    }

private:
    std::array<ResonatorVoice, kMaxVoices> voices;
    float sampleRate = 44100.0f;
    int   activeCount = 0;
};
} // namespace nscm
