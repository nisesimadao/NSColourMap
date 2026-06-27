#pragma once

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
        for (auto& t : tilt)
            t.reset();
    }

    // Update coefficients at control rate.
    void update (float formantSemitones, float amount) noexcept
    {
        const float ratio = std::pow (2.0f, formantSemitones / 12.0f);
        // Vowel-ish base formant centres.
        static constexpr std::array<float, 3> baseHz { 560.0f, 1180.0f, 2600.0f };
        const float gain = 4.0f + amount * 8.0f; // peak boost in dB

        for (int ch = 0; ch < 2; ++ch)
        {
            for (int i = 0; i < 3; ++i)
                peaks[(size_t) ch][(size_t) i].setPeak (baseHz[(size_t) i] * ratio, 3.0f, gain, sampleRate);

            // Tilt: a broad shelf-ish peak. Formant up -> brighter, down -> darker.
            const float tiltHz = formantSemitones >= 0.0f ? 4000.0f : 300.0f;
            tilt[(size_t) ch].setPeak (tiltHz, 0.7f, std::abs (formantSemitones) * 0.25f, sampleRate);
        }
        active = amount > 0.001f;
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
                    x = peaks[(size_t) ch][(size_t) i].process (x);
                x = tilt[(size_t) ch].process (x);
                data[s] = x;
            }
        }
    }

private:
    float sampleRate = 44100.0f;
    bool  active = false;
    std::array<std::array<Biquad, 3>, 2> peaks;
    std::array<Biquad, 2> tilt;
};
} // namespace nscm
