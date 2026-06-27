#pragma once

#include <cstddef>

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
        constexpr float twoPi = 6.28318530717958648f;
        // One-pole lowpass used to split highs for the shelf (~1.2 kHz corner).
        lpCoeff  = std::exp (-twoPi * 1200.0f / sampleRate);
        // Higher split for the "air" shelf (~3.8 kHz) — sparkle / brilliance.
        airCoeff = std::exp (-twoPi * 3800.0f / sampleRate);
        reset();
    }

    void reset() noexcept { lpState.fill (0.0f); airState.fill (0.0f); }

    struct Settings
    {
        float colour    = 0.5f; // 0..1 macro
        float drive     = 0.3f; // algo saturation base
        float width     = 0.4f; // 0..1 stereo spread
        float highEmph  = 0.5f; // algo high emphasis base
        float air       = 0.0f; // 0..1 high "air" shelf (shine)
    };

    void process (float* const* channels, int numChannels, int numSamples, const Settings& s) noexcept
    {
        // Keep this stage gentle: heavy saturation on the summed resonator output
        // creates intermodulation mush that masks the pitch-grid tones. Per-voice
        // drive in the ResonatorBank does the musical harmonic work instead.
        const float shelfGain = 1.0f + s.highEmph * 0.8f;                    // high shelf boost
        const float satDrive  = 1.0f + s.drive * 1.2f;                       // light saturation
        const float satComp   = 1.0f / std::sqrt (satDrive);
        const float density   = s.colour * 0.06f;                           // subtle even-harmonic add
        const float airGain   = s.air * 3.0f;                               // air shelf boost (shine)

        for (int ch = 0; ch < numChannels && ch < 2; ++ch)
        {
            float* data = channels[ch];
            float& lp   = lpState[(std::size_t) ch];
            float& ap   = airState[(std::size_t) ch];

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

                // Air shelf: brighten content above ~3.8 kHz for sparkle.
                if (airGain > 0.0f)
                {
                    ap = x + airCoeff * (ap - x);  // ap tracks lows; x-ap = highs
                    x += airGain * (x - ap);
                }

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
    float lpCoeff = 0.0f, airCoeff = 0.0f;
    std::array<float, 2> lpState {}, airState {};
};
} // namespace nscm
