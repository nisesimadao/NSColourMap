#pragma once

#include <cstddef>

#include <array>
#include <cmath>

namespace nscm
{
// A simple transposed-direct-form-II biquad.
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    float z1 = 0, z2 = 0;

    inline float process (float x) noexcept
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

    void reset() noexcept { z1 = z2 = 0.0f; }

    void setPeak (float freq, float q, float gainDb, float sampleRate) noexcept
    {
        const float nyq = sampleRate * 0.49f;
        freq = freq < 40.0f ? 40.0f : (freq > nyq ? nyq : freq);
        const float A  = std::pow (10.0f, gainDb / 40.0f);
        const float w0 = 2.0f * 3.14159265358979323846f * freq / sampleRate;
        const float cw = std::cos (w0);
        const float alpha = std::sin (w0) / (2.0f * (q < 0.1f ? 0.1f : q));

        const float a0 = 1.0f + alpha / A;
        b0 = (1.0f + alpha * A) / a0;
        b1 = (-2.0f * cw) / a0;
        b2 = (1.0f - alpha * A) / a0;
        a1 = (-2.0f * cw) / a0;
        a2 = (1.0f - alpha / A) / a0;
    }
};

// Pseudo-formant tone shaper: a few movable peak filters plus a tilt, NOT a true
// phase-vocoder formant shifter (spec §20). 'formant' is in semitones (-24..+24)
// and scales the peak centre frequencies by ratio = 2^(semitones/12).
class PseudoFormantTone
{
public:
    void prepare (double sr) noexcept
    {
        sampleRate = (float) (sr > 0.0 ? sr : 44100.0);
        reset();
    }

    void reset() noexcept
    {
        for (auto& ch : peaks)
            for (auto& p : ch)
                p.reset();
        for (auto& ch : notch)
            for (auto& n : ch)
                n.reset();
        for (auto& t : tilt)
            t.reset();
        lfoPhase = 0.0f;
    }

    // Update coefficients at control rate. 'formant' shifts vowel size, 'baseAmount'
    // is the always-on vowel presence, 'gamma' exaggerates the formant peaks, deepens
    // the anti-resonance notches between them, and slowly morphs the vowel (ah<->ee)
    // for the organic "self-modulating filter-oid" character (research: COLORS Gamma).
    void update (float formantSemitones, float baseAmount, float gamma, int numSamples) noexcept
    {
        // Real vowel formant centres (Hz): "ah" and "ee".
        static constexpr std::array<float, 3> ah { 700.0f, 1220.0f, 2600.0f };
        static constexpr std::array<float, 3> ee { 350.0f, 2000.0f, 2900.0f };

        const float ratio = std::pow (2.0f, formantSemitones / 12.0f);

        // Slow vowel morph, depth = gamma.
        lfoPhase += 6.28318530718f * 0.3f * (float) numSamples / sampleRate;
        if (lfoPhase >= 6.28318530718f) lfoPhase -= 6.28318530718f;
        const float m = gamma * (0.5f + 0.5f * std::sin (lfoPhase));

        const float q      = 4.0f + gamma * 5.0f;
        const float pkGain = 3.0f + baseAmount * 5.0f + gamma * 7.0f;     // dB
        const float nGain  = -(2.0f + gamma * 8.0f);                      // dB (anti-resonance)

        std::array<float, 3> c {};
        for (int i = 0; i < 3; ++i)
            c[(std::size_t) i] = (ah[(std::size_t) i] + m * (ee[(std::size_t) i] - ah[(std::size_t) i])) * ratio;

        for (int ch = 0; ch < 2; ++ch)
        {
            for (int i = 0; i < 3; ++i)
                peaks[(std::size_t) ch][(std::size_t) i].setPeak (c[(std::size_t) i], q, pkGain, sampleRate);

            // Anti-resonance notches at the geometric means between adjacent formants.
            notch[(std::size_t) ch][0].setPeak (std::sqrt (c[0] * c[1]), 2.5f, nGain, sampleRate);
            notch[(std::size_t) ch][1].setPeak (std::sqrt (c[1] * c[2]), 2.5f, nGain, sampleRate);

            // Tilt: formant up -> brighter, down -> darker.
            const float tiltHz = formantSemitones >= 0.0f ? 4000.0f : 300.0f;
            tilt[(std::size_t) ch].setPeak (tiltHz, 0.7f, std::abs (formantSemitones) * 0.2f, sampleRate);
        }
        active = (baseAmount > 0.001f) || (gamma > 0.001f) || (std::abs (formantSemitones) > 0.01f);
    }

    void process (float* const* channels, int numChannels, int numSamples) noexcept
    {
        if (! active)
            return;

        for (int ch = 0; ch < numChannels && ch < 2; ++ch)
        {
            float* data = channels[ch];
            for (int s = 0; s < numSamples; ++s)
            {
                float x = data[s];
                for (int i = 0; i < 3; ++i)
                    x = peaks[(std::size_t) ch][(std::size_t) i].process (x);
                x = notch[(std::size_t) ch][0].process (x);
                x = notch[(std::size_t) ch][1].process (x);
                x = tilt[(std::size_t) ch].process (x);
                data[s] = x;
            }
        }
    }

private:
    float sampleRate = 44100.0f;
    float lfoPhase = 0.0f;
    bool  active = false;
    std::array<std::array<Biquad, 3>, 2> peaks;
    std::array<std::array<Biquad, 2>, 2> notch;
    std::array<Biquad, 2> tilt;
};
} // namespace nscm
