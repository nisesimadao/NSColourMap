#pragma once

#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <cstdint>

namespace nscm
{
// STFT pitch-class mapper (spec §12.3 / Phase 9 v2). Overlap-add FFT that snaps
// the spectrum to the grid: bins whose frequency maps to an in-grid pitch class
// are kept (and gently emphasised); off-grid bins are attenuated. This is the
// "true" PITCHMAP/Chroma-style spectral snap, used as the High Quality engine
// (it has fftSize latency, so 0-Latency mode keeps the oscillator core instead).
class SpectralMapper
{
public:
    void prepare (double sr, int order, int numChannels) noexcept
    {
        sampleRate = (float) (sr > 0.0 ? sr : 44100.0);
        fftOrder   = order;
        fftSize    = 1 << order;
        hop        = fftSize / 4;                    // 75% overlap
        fft        = std::make_unique<juce::dsp::FFT> (order);

        window.resize ((size_t) fftSize);
        for (int i = 0; i < fftSize; ++i)            // Hann
            window[(size_t) i] = 0.5f - 0.5f * std::cos (2.0f * 3.14159265358979f * (float) i / (float) fftSize);

        // OLA gain for a Hann window applied on analysis AND synthesis at 75% overlap
        // (sum of Hann^2 across 4x-overlapping frames = 1.5).
        olaGain = 1.0f / 1.5f;

        const int nc = juce::jmax (1, numChannels);
        chans.assign ((size_t) nc, {});
        for (auto& c : chans)
        {
            c.inRing.assign ((size_t) fftSize, 0.0f);
            c.outRing.assign ((size_t) fftSize, 0.0f);
            c.fftData.assign ((size_t) (2 * fftSize), 0.0f);
            c.ringPos = 0;
            c.hopCount = 0;
        }
    }

    int getLatency() const noexcept { return fftSize; }

    void reset() noexcept
    {
        for (auto& c : chans)
        {
            std::fill (c.inRing.begin(),  c.inRing.end(),  0.0f);
            std::fill (c.outRing.begin(), c.outRing.end(), 0.0f);
            c.ringPos = 0; c.hopCount = 0;
        }
    }

    void setGrid (std::uint16_t pitchMask) noexcept { mask = pitchMask; }
    void setStrength (float s) noexcept { strength = s < 0.0f ? 0.0f : (s > 1.0f ? 1.0f : s); }

    // Always runs the OLA so the latency stays constant (passes through when mask==0).
    void process (float* const* channels, int numChannels, int numSamples) noexcept
    {
        const int nc = juce::jmin (numChannels, (int) chans.size());
        for (int ch = 0; ch < nc; ++ch)
        {
            auto& c = chans[(size_t) ch];
            float* data = channels[ch];

            for (int n = 0; n < numSamples; ++n)
            {
                c.inRing[(size_t) c.ringPos] = data[n];
                data[n] = c.outRing[(size_t) c.ringPos];
                c.outRing[(size_t) c.ringPos] = 0.0f;
                c.ringPos = (c.ringPos + 1) % fftSize;

                if (++c.hopCount >= hop)
                {
                    c.hopCount = 0;
                    processFrame (c);
                }
            }
        }
    }

private:
    struct Chan
    {
        std::vector<float> inRing, outRing, fftData;
        int ringPos = 0, hopCount = 0;
    };

    void processFrame (Chan& c) noexcept
    {
        // Windowed copy of the latest fftSize samples (oldest at ringPos).
        for (int j = 0; j < fftSize; ++j)
            c.fftData[(size_t) j] = c.inRing[(size_t) ((c.ringPos + j) % fftSize)] * window[(size_t) j];
        std::fill (c.fftData.begin() + fftSize, c.fftData.end(), 0.0f);

        fft->performRealOnlyForwardTransform (c.fftData.data());

        // Snap to grid: keep a bin if it CONTAINS an in-scale note (so coarse low-freq
        // bins that span a scale note are preserved instead of wrongly gated), else
        // attenuate. This avoids damaging the bass where bin resolution is low.
        if (mask != 0)
        {
            const float binHz = sampleRate / (float) fftSize;
            const float half  = binHz * 0.5f;
            const float off   = 1.0f - strength;             // off-grid retention
            const float in    = 1.0f + 0.3f * strength;      // gentle in-grid emphasis
            for (int k = 1; k < fftSize / 2; ++k)
            {
                const float freq = (float) k * binHz;
                bool keep = false;
                if (freq >= 20.0f)
                {
                    const float midi = 12.0f * std::log2 (freq / 16.351597f); // semitones from C0
                    const int   base = (int) std::lround (midi);
                    for (int d = 0; d <= 6 && ! keep; ++d)
                        for (int sgn = -1; sgn <= 1 && ! keep; sgn += 2)
                        {
                            const int s  = base + sgn * d;
                            const int pc = ((s % 12) + 12) % 12;
                            if (! (mask & (1u << pc))) continue;
                            const float fn = 16.351597f * std::pow (2.0f, (float) s / 12.0f);
                            if (std::abs (freq - fn) < half) keep = true; // bin contains this scale note
                            if (d == 0) break;
                        }
                }
                const float wgt = keep ? in : off;
                c.fftData[(size_t) (2 * k)]     *= wgt;
                c.fftData[(size_t) (2 * k + 1)] *= wgt;
            }
            c.fftData[0] *= off;                              // DC
            c.fftData[1] *= off;                              // Nyquist (packed in imag of bin 0)
        }

        fft->performRealOnlyInverseTransform (c.fftData.data());

        // Windowed overlap-add back into the output ring.
        for (int j = 0; j < fftSize; ++j)
            c.outRing[(size_t) ((c.ringPos + j) % fftSize)] += c.fftData[(size_t) j] * window[(size_t) j] * olaGain;
    }

    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> window;
    std::vector<Chan>  chans;
    float sampleRate = 44100.0f;
    float olaGain = 1.0f, strength = 0.8f;
    int   fftOrder = 10, fftSize = 1024, hop = 256;
    std::uint16_t mask = 0;
};
} // namespace nscm
