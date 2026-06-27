#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <vector>
#include <cmath>

namespace nscm
{
// Lightweight FFT spectrum analyzer for the UI (like an EQ's frequency display).
// The audio thread feeds output samples; it computes a windowed magnitude
// spectrum and writes log-spaced, smoothed display bins to atomics the UI reads.
class SpectrumAnalyzer
{
public:
    static constexpr int kBins = 128;        // display points
    static constexpr float fLo = 30.0f, fHi = 18000.0f;

    void prepare (double sr, int order) noexcept
    {
        sampleRate = (float) (sr > 0.0 ? sr : 44100.0);
        fftOrder = order;
        fftSize  = 1 << order;
        hop      = fftSize / 2;
        fft      = std::make_unique<juce::dsp::FFT> (order);

        window.resize ((size_t) fftSize);
        for (int i = 0; i < fftSize; ++i)
            window[(size_t) i] = 0.5f - 0.5f * std::cos (2.0f * 3.14159265358979f * (float) i / (float) fftSize);

        ring.assign ((size_t) fftSize, 0.0f);
        fftData.assign ((size_t) (2 * fftSize), 0.0f);
        ringPos = 0; hopCount = 0;
        for (auto& b : bins) b.store (0.0f);
    }

    void reset() noexcept
    {
        std::fill (ring.begin(), ring.end(), 0.0f);
        ringPos = 0; hopCount = 0;
        for (auto& b : bins) b.store (0.0f);
    }

    // Feed the (stereo) output; averaged to mono for analysis.
    void pushBlock (const float* const* channels, int numChannels, int numSamples) noexcept
    {
        const float norm = numChannels > 0 ? 1.0f / (float) numChannels : 1.0f;
        for (int n = 0; n < numSamples; ++n)
        {
            float mono = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch) mono += channels[ch][n];
            ring[(size_t) ringPos] = mono * norm;
            ringPos = (ringPos + 1) % fftSize;
            if (++hopCount >= hop) { hopCount = 0; analyze(); }
        }
    }

    float getBin (int i) const noexcept
    {
        return (i >= 0 && i < kBins) ? bins[(size_t) i].load() : 0.0f;
    }

private:
    void analyze() noexcept
    {
        for (int j = 0; j < fftSize; ++j)
            fftData[(size_t) j] = ring[(size_t) ((ringPos + j) % fftSize)] * window[(size_t) j];
        std::fill (fftData.begin() + fftSize, fftData.end(), 0.0f);

        fft->performFrequencyOnlyForwardTransform (fftData.data()); // magnitudes in [0..fftSize/2]

        const float binHz   = sampleRate / (float) fftSize;
        const float logRange = std::log (fHi / fLo);
        const float refScale = 2.0f / (float) fftSize;

        for (int i = 0; i < kBins; ++i)
        {
            const float f  = fLo * std::exp (logRange * (float) i / (float) (kBins - 1));
            const int   k0 = (int) (f / binHz);
            const float fw = f * 1.06f;                 // ~ one display step up
            const int   k1 = juce::jlimit (1, fftSize / 2 - 1, (int) (fw / binHz));
            float mag = 0.0f;
            for (int k = juce::jlimit (1, fftSize / 2 - 1, k0); k <= k1; ++k)
                mag = std::max (mag, fftData[(size_t) k]);

            const float db   = 20.0f * std::log10 (mag * refScale + 1.0e-6f);
            const float norm = juce::jlimit (0.0f, 1.0f, (db + 84.0f) / 84.0f); // -84..0 dB -> 0..1
            // Peak-ish smoothing: fast rise, slow fall.
            const float prev = bins[(size_t) i].load();
            const float out  = norm > prev ? norm : prev * 0.82f + norm * 0.18f;
            bins[(size_t) i].store (out);
        }
    }

    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> window, ring, fftData;
    std::array<std::atomic<float>, kBins> bins {};
    float sampleRate = 44100.0f;
    int   fftOrder = 11, fftSize = 2048, hop = 1024, ringPos = 0, hopCount = 0;
};
} // namespace nscm
