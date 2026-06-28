#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <vector>
#include <cmath>
#include <cstdint>

namespace nscm
{
// Detects the dominant pitch classes in the incoming AUDIO (a chromagram) and
// exposes them as a 12-bit grid mask — this drives the "Audio" grid mode, where
// the colour follows whatever notes the input is actually playing (no MIDI).
class ChromaDetector
{
public:
    void prepare (double sr, int order) noexcept
    {
        sampleRate = (float) (sr > 0.0 ? sr : 44100.0);
        fftSize = 1 << order;
        hop     = fftSize / 2;
        fft     = std::make_unique<juce::dsp::FFT> (order);

        window.resize ((size_t) fftSize);
        for (int i = 0; i < fftSize; ++i)
            window[(size_t) i] = 0.5f - 0.5f * std::cos (2.0f * 3.14159265358979f * (float) i / (float) fftSize);

        ring.assign ((size_t) fftSize, 0.0f);
        fftData.assign ((size_t) (2 * fftSize), 0.0f);
        ringPos = 0; hopCount = 0;
        chroma.fill (0.0f);
        mask.store (0);
    }

    void reset() noexcept
    {
        std::fill (ring.begin(), ring.end(), 0.0f);
        ringPos = 0; hopCount = 0;
        chroma.fill (0.0f);
        mask.store (0);
    }

    void push (const float* const* channels, int numChannels, int numSamples) noexcept
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

    std::uint16_t getMask() const noexcept { return mask.load(); }

private:
    void analyze() noexcept
    {
        for (int j = 0; j < fftSize; ++j)
            fftData[(size_t) j] = ring[(size_t) ((ringPos + j) % fftSize)] * window[(size_t) j];
        std::fill (fftData.begin() + fftSize, fftData.end(), 0.0f);
        fft->performFrequencyOnlyForwardTransform (fftData.data());

        const float binHz = sampleRate / (float) fftSize;
        std::array<float, 12> c {};
        for (int k = 1; k < fftSize / 2; ++k)
        {
            const float freq = (float) k * binHz;
            if (freq < 55.0f || freq > 2000.0f)        // fundamental range -> cleaner chroma
                continue;
            const float mag = fftData[(size_t) k];
            const int pc = ((int) std::lround (12.0f * std::log2 (freq / 16.351597f)) % 12 + 12) % 12;
            c[(size_t) pc] += mag;
        }

        // Smooth the chromagram over time for stability.
        float maxv = 0.0f;
        for (int i = 0; i < 12; ++i)
        {
            chroma[(size_t) i] = chroma[(size_t) i] * 0.80f + c[(size_t) i] * 0.20f;
            if (chroma[(size_t) i] > maxv) maxv = chroma[(size_t) i];
        }

        // Build the mask from pitch classes that stand out (relative threshold).
        std::uint16_t m = 0;
        if (maxv > 1.0e-4f)
        {
            const float thr = maxv * 0.45f;
            for (int i = 0; i < 12; ++i)
                if (chroma[(size_t) i] >= thr)
                    m |= (std::uint16_t) (1u << i);
        }
        mask.store (m);
    }

    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> window, ring, fftData;
    std::array<float, 12> chroma {};
    std::atomic<std::uint16_t> mask { 0 };
    float sampleRate = 44100.0f;
    int   fftSize = 2048, hop = 1024, ringPos = 0, hopCount = 0;
};
} // namespace nscm
