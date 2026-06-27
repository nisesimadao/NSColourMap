#pragma once

#include <juce_dsp/juce_dsp.h>

namespace nscm
{
// Splits the signal into a protected low/sub band and a mid/high band that is
// sent through the colour processing (spec §16). Uses a Linkwitz-Riley 4th order
// crossover so the bands recombine flat. The interface is deliberately small so
// the implementation can be swapped if needed.
class SubSplitter
{
public:
    void prepare (double sampleRate, int /*maxBlock*/, int numChannels) noexcept
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate       = sampleRate > 0.0 ? sampleRate : 44100.0;
        spec.maximumBlockSize = 512;
        spec.numChannels      = (juce::uint32) juce::jmax (1, numChannels);

        crossover.prepare (spec);
        crossover.setType (juce::dsp::LinkwitzRileyFilterType::lowpass);
        crossover.setCutoffFrequency (currentFreq);
        crossover.reset();
    }

    void setCrossoverFrequency (float hz) noexcept
    {
        hz = juce::jlimit (30.0f, 320.0f, hz);
        if (std::abs (hz - currentFreq) > 0.01f)
        {
            currentFreq = hz;
            crossover.setCutoffFrequency (currentFreq);
        }
    }

    // Splits in-place: 'low' receives the protected band, 'high' the processed band.
    void process (const float* const* input, float* const* low, float* const* high,
                  int numChannels, int numSamples) noexcept
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            for (int s = 0; s < numSamples; ++s)
            {
                float lo = 0.0f, hi = 0.0f;
                crossover.processSample (ch, input[ch][s], lo, hi);
                low[ch][s]  = lo;
                high[ch][s] = hi;
            }
        }
    }

    void reset() noexcept { crossover.reset(); }

    float getFrequency() const noexcept { return currentFreq; }

private:
    juce::dsp::LinkwitzRileyFilter<float> crossover;
    float currentFreq = 120.0f;
};
} // namespace nscm
