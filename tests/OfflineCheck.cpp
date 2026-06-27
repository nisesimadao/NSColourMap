// Offline host: instantiates the real plugin and measures whether processBlock
// actually changes the audio. This exercises the FULL chain (not just the core).

#include "PluginProcessor.h"

#include <cstdio>
#include <cmath>
#include <vector>
#include <random>

static double rms (const float* x, int n) { double a = 0; for (int i = 0; i < n; ++i) a += (double) x[i] * x[i]; return std::sqrt (a / n); }

static double bandRms (const float* x, int n, double freq, double sr)
{
    nscm::SvfResonator det; det.setCoeffs ((float) freq, 20.0f, (float) sr);
    double a = 0; for (int i = 0; i < n; ++i) { float bp = det.processBandpass (x[i]); a += (double) bp * bp; }
    return std::sqrt (a / n);
}

static void setP (juce::AudioProcessorValueTreeState& s, const char* id, float value)
{
    if (auto* p = s.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (value));
}

int main()
{
    const double sr = 48000.0;
    const int    block = 512;
    const int    N = 48000;

    NSColourMapAudioProcessor proc;
    proc.prepareToPlay (sr, block);

    auto& s = proc.getState();
    setP (s, nscm::params::mode,      0.0f);   // Scale
    setP (s, nscm::params::character, 1.0f);   // Color
    setP (s, nscm::params::key,       0.0f);   // C
    setP (s, nscm::params::scale,     7.0f);   // Minor Pentatonic
    setP (s, nscm::params::transient, 0.0f);   // isolate the colour effect for the test

    // Choose config via argv: default (out-of-box) vs maxed.
    const bool maxed = false;  // measure OUT-OF-BOX defaults (the user's first impression)
    if (maxed)
    {
        setP (s, nscm::params::color,  2.0f);
        setP (s, nscm::params::amount, 1.0f);
        setP (s, nscm::params::mix,    1.0f);
    }
    // else: leave COLOR=65%, Amount=70%, Mix=65% (spec defaults).

    // Harmonic input: sawtooth at A2 = 110 Hz (A is OFF the C-minor-pentatonic
    // scale, so a working colour mapper must move energy onto C/Eb/F/G/Bb).
    const double sawHz = 110.0;
    long phase = 0;

    juce::AudioBuffer<float> buffer (2, block);
    juce::MidiBuffer midi;

    std::vector<float> inAll, outAll;
    inAll.reserve (N); outAll.reserve (N);

    int done = 0;
    while (done < N)
    {
        const int n = std::min (block, N - done);
        buffer.setSize (2, n, false, false, true);
        for (int i = 0; i < n; ++i)
        {
            const double t = std::fmod ((double) (phase++) * sawHz / sr, 1.0);
            const float saw = (float) (2.0 * t - 1.0) * 0.3f;
            buffer.getWritePointer (0)[i] = saw;
            buffer.getWritePointer (1)[i] = saw;
        }
        // capture input (ch0)
        for (int i = 0; i < n; ++i) inAll.push_back (buffer.getReadPointer (0)[i]);

        proc.processBlock (buffer, midi);

        for (int i = 0; i < n; ++i) outAll.push_back (buffer.getReadPointer (0)[i]);
        done += n;
    }

    // Analyse settled second half.
    const int h = N / 2;
    const float* in  = inAll.data()  + h;
    const float* out = outAll.data() + h;
    const int    m   = N - h;

    const double inR  = rms (in, m);
    const double outR = rms (out, m);

    // difference signal
    std::vector<float> diff (m);
    for (int i = 0; i < m; ++i) diff[(size_t) i] = out[i] - in[i];
    const double diffR = rms (diff.data(), m);

    // Tonality: sum band energy at all in-scale pitch classes vs off-scale, across
    // octaves. C Minor Pentatonic = {C, Eb, F, G, Bb} = {0,3,5,7,10}.
    auto pcInScale = [] (int pc) { return pc == 0 || pc == 3 || pc == 5 || pc == 7 || pc == 10; };
    double inScale = 0.0, offScale = 0.0;
    for (int note = 48; note <= 84; ++note)
    {
        const double hz = 440.0 * std::pow (2.0, (note - 69) / 12.0);
        const double e  = bandRms (out, m, hz, sr);
        if (pcInScale (note % 12)) inScale += e; else offScale += e;
    }
    // 5 in-scale vs 7 off-scale pitch classes -> normalise per-class.
    const double inAvg  = inScale / 5.0 / 3.0;   // 3 octaves
    const double offAvg = offScale / 7.0 / 3.0;

    printf ("input  RMS : %.4f\n", inR);
    printf ("output RMS : %.4f  (%.2fx)\n", outR, outR / (inR + 1e-9));
    printf ("diff   RMS : %.4f  (%.1f%% of input)\n", diffR, 100.0 * diffR / (inR + 1e-9));
    // Brightness: energy above ~3.5 kHz (one-pole highpass), out vs in.
    auto highRms = [sr] (const float* x, int n) {
        const float c = std::exp (-2.0f * 3.14159265f * 3500.0f / (float) sr);
        float lp = 0.0f; double a = 0.0;
        for (int i = 0; i < n; ++i) { lp = x[i] + c * (lp - x[i]); const double h = x[i] - lp; a += h * h; }
        return std::sqrt (a / n);
    };
    const double hiIn  = highRms (in, m);
    const double hiOut = highRms (out, m);
    printf ("HF(>3.5k) in : %.4f   out : %.4f   (%.2fx brighter)\n", hiIn, hiOut, hiOut / (hiIn + 1e-9));
    printf ("in-scale avg energy  : %.4f\n", inAvg);
    printf ("off-scale avg energy : %.4f\n", offAvg);
    printf ("in/off tonality ratio: %.2f\n", inAvg / (offAvg + 1e-9));
    printf ("voices: %d  targetMask: 0x%03x\n", proc.getActiveVoiceCount(), proc.getTargetMask());

    const bool changed = diffR > 0.10 * inR;
    const bool tonal   = inAvg > 1.5 * offAvg;
    printf ("\nCHANGED=%d TONAL=%d\n", changed, tonal);
    return (changed && tonal) ? 0 : 1;
}
