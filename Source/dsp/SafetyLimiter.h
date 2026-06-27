#pragma once

#include <cmath>

namespace nscm
{
// Final clip guard so aggressive Colour / Broken settings never hard-clip the
// output. A soft-knee tanh limiter above a threshold (spec §11.2 / §31).
class SafetyLimiter
{
public:
    void prepare (double /*sr*/) noexcept {}
    void reset() noexcept {}

    void process (float* const* channels, int numChannels, int numSamples) noexcept
    {
        constexpr float threshold = 0.98f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* data = channels[ch];
            for (int s = 0; s < numSamples; ++s)
            {
                float x = data[s];
                const float a = std::abs (x);
                if (a > threshold)
                {
                    const float sign = x < 0.0f ? -1.0f : 1.0f;
                    const float over = a - threshold;
                    x = sign * (threshold + (1.0f - threshold) * std::tanh (over / (1.0f - threshold)));
                }
                data[s] = x;
            }
        }
    }
};
} // namespace nscm
