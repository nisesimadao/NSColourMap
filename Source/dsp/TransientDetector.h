#pragma once

#include <cmath>

namespace nscm
{
// Detects fast energy increases (attacks) so the wet processing can be bypassed
// for transients and the dry attack mixed back in (spec §17). Produces a 0..1
// envelope per sample. Detection is on a mono sum of the mid/high band.
class TransientDetector
{
public:
    void prepare (double sr) noexcept
    {
        sampleRate = (float) (sr > 0.0 ? sr : 44100.0);
        fastCoeff = std::exp (-1.0f / (0.0015f * sampleRate)); // ~1.5 ms
        slowCoeff = std::exp (-1.0f / (0.080f  * sampleRate)); // ~80 ms
        relCoeff  = std::exp (-1.0f / (0.030f  * sampleRate)); // ~30 ms transient release
        fastEnv = slowEnv = transient = 0.0f;
    }

    void reset() noexcept { fastEnv = slowEnv = transient = 0.0f; }

    // Fills 'out' with a 0..1 transient envelope. 'flash' returns the block peak
    // for UI feedback.
    void process (const float* const* input, int numChannels, int numSamples,
                  float* out, float& flash) noexcept
    {
        float peak = 0.0f;
        for (int s = 0; s < numSamples; ++s)
        {
            float mono = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch)
                mono += input[ch][s];
            mono = std::abs (mono / (float) (numChannels > 0 ? numChannels : 1));

            fastEnv = mono + fastCoeff * (fastEnv - mono);
            slowEnv = mono + slowCoeff * (slowEnv - mono);

            const float diff = fastEnv - slowEnv;
            float target = diff > 0.0f ? diff / (slowEnv + 0.02f) : 0.0f;
            target = target > 1.0f ? 1.0f : target;

            // Fast attack, slower release so the transient window stays open briefly.
            transient = target > transient ? target : target + relCoeff * (transient - target);
            out[s] = transient;
            if (transient > peak)
                peak = transient;
        }
        flash = peak;
    }

private:
    float sampleRate = 44100.0f;
    float fastCoeff = 0.0f, slowCoeff = 0.0f, relCoeff = 0.0f;
    float fastEnv = 0.0f, slowEnv = 0.0f, transient = 0.0f;
};
} // namespace nscm
