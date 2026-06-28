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

    // ── MIDI note-off must stop the colour (Freeze off) ───────────────────────
    {
        NSColourMapAudioProcessor mp;
        mp.prepareToPlay (sr, block);
        auto& ms = mp.getState();
        setP (ms, nscm::params::mode,       1.0f); // MIDI Grid
        setP (ms, nscm::params::character,  1.0f);
        setP (ms, nscm::params::midiFreeze, 0.0f); // off -> release should stop
        setP (ms, nscm::params::color,      1.0f);
        setP (ms, nscm::params::mix,        1.0f);
        setP (ms, nscm::params::transient,  0.0f);

        std::vector<float> mIn, mOut;
        long ph = 0; int d = 0;
        while (d < N)
        {
            const int n = std::min (block, N - d);
            juce::AudioBuffer<float> buf (2, n);
            for (int i = 0; i < n; ++i)
            {
                const double t = std::fmod ((double) (ph++) * 110.0 / sr, 1.0);
                const float saw = (float) (2.0 * t - 1.0) * 0.3f;
                buf.getWritePointer (0)[i] = saw; buf.getWritePointer (1)[i] = saw;
            }
            juce::MidiBuffer mb;
            if (d == 0) mb.addEvent (juce::MidiMessage::noteOn (1, 48, (juce::uint8) 100), 0);  // hold C3
            if (d <= N / 2 && d + n > N / 2) mb.addEvent (juce::MidiMessage::noteOff (1, 48), 0); // release at half
            for (int i = 0; i < n; ++i) mIn.push_back (buf.getReadPointer (0)[i]);
            mp.processBlock (buf, mb);
            for (int i = 0; i < n; ++i) mOut.push_back (buf.getReadPointer (0)[i]);
            d += n;
        }
        // held quarter (note on) vs released quarter (well after note-off)
        auto diffPct = [&] (int a, int b) {
            double di = 0, ii = 0;
            for (int i = a; i < b; ++i) { const double e = mOut[(size_t) i] - mIn[(size_t) i]; di += e * e; ii += (double) mIn[(size_t) i] * mIn[(size_t) i]; }
            return 100.0 * std::sqrt (di / (b - a)) / (std::sqrt (ii / (b - a)) + 1e-9);
        };
        const double heldDiff     = diffPct (N / 4, N / 2);          // colour active
        const double releasedDiff = diffPct (3 * N / 4, N);         // should be ~0
        printf ("\n[MIDI release] held diff: %.1f%%   released diff: %.1f%%\n", heldDiff, releasedDiff);
        const bool stops = releasedDiff < 3.0 && heldDiff > 15.0;
        printf ("MIDI_STOPS_ON_RELEASE=%d\n", stops);
        juce::ignoreUnused (stops);
    }

    // ── High Quality STFT spectral snap: off-scale energy must drop ───────────
    {
        NSColourMapAudioProcessor hp;
        hp.prepareToPlay (sr, block);
        auto& hs = hp.getState();
        setP (hs, nscm::params::mode,      0.0f);  // Scale
        setP (hs, nscm::params::character, 1.0f);
        setP (hs, nscm::params::key,       0.0f);
        setP (hs, nscm::params::scale,     7.0f);  // C minor pentatonic
        setP (hs, nscm::params::quality,   1.0f);  // High Quality -> STFT engine
        setP (hs, nscm::params::color,     1.0f);
        setP (hs, nscm::params::amount,    1.0f);
        setP (hs, nscm::params::mix,       1.0f);

        std::vector<float> outv; long ph = 0; int d = 0;
        bool finite = true;
        while (d < N)
        {
            const int n = std::min (block, N - d);
            juce::AudioBuffer<float> b (2, n);
            for (int i = 0; i < n; ++i)
            {
                const double t = std::fmod ((double) (ph++) * 110.0 / sr, 1.0); // A2 (off-scale)
                const float saw = (float) (2.0 * t - 1.0) * 0.3f;
                b.getWritePointer (0)[i] = saw; b.getWritePointer (1)[i] = saw;
            }
            juce::MidiBuffer mb; hp.processBlock (b, mb);
            for (int i = 0; i < n; ++i)
            {
                const float v = b.getReadPointer (0)[i];
                if (! std::isfinite (v)) finite = false;
                outv.push_back (v);
            }
            d += n;
        }
        const int hh = N / 2; const int mm = N - hh;
        auto pcIn = [] (int pc) { return pc == 0 || pc == 3 || pc == 5 || pc == 7 || pc == 10; };
        double isc = 0, osc = 0;
        for (int note = 48; note <= 84; ++note)
        {
            const double hz = 440.0 * std::pow (2.0, (note - 69) / 12.0);
            const double e = bandRms (outv.data() + hh, mm, hz, sr);
            if (pcIn (note % 12)) isc += e; else osc += e;
        }
        const double hqTon = (isc / 5.0) / (osc / 7.0 + 1e-9);
        printf ("\n[HQ STFT] finite=%d   in/off tonality: %.2f\n", (int) finite, hqTon);
        printf ("HQ_OK=%d\n", (int) (finite && hqTon > 1.5));
        juce::ignoreUnused (finite, hqTon);
    }

    // ── Latency probe: actual signal delay vs reported latency ────────────────
    for (int q = 0; q <= 1; ++q)
    {
        NSColourMapAudioProcessor lp;
        auto& ls = lp.getState();
        setP (ls, nscm::params::quality, (float) q);    // set BEFORE prepare (mimics DAW state restore)
        lp.prepareToPlay (sr, block);
        const int latAtPrepare = lp.getLatencySamples(); // what offline render reads
        setP (ls, nscm::params::mode,    0.0f);  // Scale
        setP (ls, nscm::params::color,   0.0f);  // pure passthrough -> output should equal input
        setP (ls, nscm::params::amount,  0.0f);
        setP (ls, nscm::params::mix,     1.0f);
        setP (ls, nscm::params::lowCut,  40.0f);
        setP (ls, nscm::params::highCut, 16000.0f);

        const int total = 8192;
        const int imp = 64;
        std::vector<float> outv; outv.reserve (total);
        int d = 0; int reported = -1;
        while (d < total)
        {
            const int n = std::min (block, total - d);
            juce::AudioBuffer<float> b (2, n);
            b.clear();
            for (int i = 0; i < n; ++i)
                if (d + i == imp) { b.setSample (0, i, 1.0f); b.setSample (1, i, 1.0f); }
            juce::MidiBuffer mb; lp.processBlock (b, mb);
            reported = lp.getLatencySamples();
            for (int i = 0; i < n; ++i) outv.push_back (b.getReadPointer (0)[i]);
            d += n;
        }
        int argmax = 0; float peak = 0.0f;
        for (int i = 0; i < (int) outv.size(); ++i) { const float a = std::abs (outv[(size_t) i]); if (a > peak) { peak = a; argmax = i; } }
        const int actualLatency = argmax - imp;
        printf ("\n[Latency q=%d (%s)] latAtPrepare=%d  reported=%d  actual=%d  peak=%.3f  %s\n",
                q, q == 0 ? "0 Latency" : "High Quality", latAtPrepare, reported, actualLatency, peak,
                (actualLatency == latAtPrepare) ? "OK (prepare matches actual)" : "MISMATCH");
    }

    return (changed && tonal) ? 0 : 1;
}
