#pragma once

#include <array>
#include <cmath>

namespace nscm
{
// Adds the "colour bass" character on top of the resonator bank: gentle
// saturation, high-harmonic emphasis, a touch of harmonic density and stereo
// spread (spec §19). All driven by the single Colour macro plus per-algo amount.
class ColourProcessor
{
public:
    void prepare (double sr) noexcept
    {
        sampleRate = (float) (sr > 0.0 ? sr : 44100.0);
        // One-pole lowpass used to split highs for the shelf (~1.2 kHz corner).
        const float fc = 1200.0f;
        lpCoeff = std::exp (-2.0f * 3.14159265358979323846f * fc / sampleRate);
        reset();
    }

    void reset() noexcept { lpState.fill (0.0f); }

    struct Settings
    {
        float colour    = 0.5f; // 0..1 macro
        float drive     = 0.3f; // algo saturation base
        float width     = 0.4f; // 0..1 stereo spread
        float highEmph  = 0.5f; // algo high emphasis base
    };

    void process (float* const* channels, int numChannels, int numSamples, const Settings& s) noexcept
    {
        const float shelfGain = 1.0f + (s.highEmph + s.colour) * 1.6f;       // high shelf boost
        const float satDrive  = 1.0f + (s.drive + s.colour * 0.6f) * 4.0f;    // saturation
        const float satComp   = 1.0f / std::sqrt (satDrive);
        const float density   = s.colour * 0.25f;                            // even-harmonic add

        for (int ch = 0; ch < numChannels && ch < 2; ++ch)
        {
            float* data = channels[ch];
            float& lp   = lpState[(size_t) ch];

            for (int s2 = 0; s2 < numSamples; ++s2)
            {
                float x = data[s2];

                // High-shelf emphasis (boost content above the lowpass corner).
                lp = x + lpCoeff * (lp - x);
                const float high = x - lp;
                x = lp + high * shelfGain;

                // Gentle saturation.
                x = std::tanh (x * satDrive) * satComp;

                // Subtle harmonic density (asymmetric -> even harmonics).
                x += density * (x * x - 0.5f * x);

                data[s2] = x;
            }
        }

        // Stereo width via mid/side (highs only signal already).
        if (numChannels >= 2 && s.width > 0.0f)
        {
            const float wide = 1.0f + s.width;
            float* l = channels[0];
            float* r = channels[1];
            for (int s2 = 0; s2 < numSamples; ++s2)
            {
                const float mid  = 0.5f * (l[s2] + r[s2]);
                const float side = 0.5f * (l[s2] - r[s2]) * wide;
                l[s2] = mid + side;
                r[s2] = mid - side;
            }
        }
    }

private:
    float sampleRate = 44100.0f;
    float lpCoeff = 0.0f;
    std::array<float, 2> lpState {};
};
} // namespace nscm
