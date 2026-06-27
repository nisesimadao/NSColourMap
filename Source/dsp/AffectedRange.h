#pragma once

#include <juce_dsp/juce_dsp.h>

namespace nscm
{
// Isolates the "Affected Frequency Range" that gets coloured (spec §6.4 / §9.9).
// Everything outside [lowCut, highCut] — including the protected sub — bypasses
// the colour engine and stays dry. Implemented as a Linkwitz-Riley highpass
// (lowCut) followed by a lowpass (highCut).
class AffectedRange
{
public:
    void prepare (double sampleRate, int maxBlock, int numChannels) noexcept
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate       = sampleRate > 0.0 ? sampleRate : 44100.0;
        spec.maximumBlockSize = (juce::uint32) juce::jmax (16, maxBlock);
        spec.numChannels      = (juce::uint32) juce::jmax (1, numChannels);

        hp.prepare (spec);
        lp.prepare (spec);
        hp.setType (juce::dsp::LinkwitzRileyFilterType::highpass);
        lp.setType (juce::dsp::LinkwitzRileyFilterType::lowpass);
        setRange (lowHz, highHz);
        reset();
    }

    void reset() noexcept { hp.reset(); lp.reset(); }

    void setRange (float lowCutHz, float highCutHz) noexcept
    {
        lowHz  = juce::jlimit (20.0f, 1000.0f, lowCutHz);
        highHz = juce::jlimit (lowHz * 1.2f, 20000.0f, highCutHz);
        hp.setCutoffFrequency (lowHz);
        lp.setCutoffFrequency (highHz);
    }

    // Writes the in-range band into 'active'. The caller keeps the original input
    // to recombine ( out = input + mix * (processed - active) ).
    void process (const float* const* input, float* const* active, int numChannels, int numSamples) noexcept
    {
        for (int ch = 0; ch < numChannels; ++ch)
            for (int s = 0; s < numSamples; ++s)
                active[ch][s] = lp.processSample (ch, hp.processSample (ch, input[ch][s]));
    }

    float getLowHz()  const noexcept { return lowHz; }
    float getHighHz() const noexcept { return highHz; }

private:
    juce::dsp::LinkwitzRileyFilter<float> hp, lp;
    float lowHz = 100.0f, highHz = 6000.0f;
};
} // namespace nscm
