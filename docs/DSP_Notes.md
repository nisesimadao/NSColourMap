# DSP Notes

All DSP classes live in `Source/dsp/` as header-only, UI-free, allocation-free
(on the audio thread) units. Each is usable without the plugin.

## Signal flow (spec §13.1)

```
Audio In
 → copy to dryBuf (original)
 → SubSplitter (Linkwitz-Riley LR4)  ──► low band  (protected, stays dry)
                                      └─► high band (processed)
       high → dryHighBuf (kept for transient return + dry/wet)
       high → TransientDetector  (fills 0..1 envelope)
       high → ResonatorBank → ColourProcessor → PseudoFormantTone
 → per-sample combine:
       effHigh = dryHigh + amount * (wetHigh - dryHigh)      (effect depth)
       effHigh = effHigh + tGain * (dryHigh - effHigh)       (transient bypass)
       wet     = low + effHigh                               (sub protected)
       out     = dryFull + mix * (wet - dryFull)             (dry/wet)
       out    *= outputGain
 → SafetyLimiter (soft tanh knee) → Audio Out
```

Because the protected low band appears identically in both the dry and the wet
signal, the Mix control never thins the sub — that is the Sub Protect guarantee.

## Classes

| Class | Role |
|---|---|
| `ScaleNoteSet` | Key + scale → 12-bit pitch-class mask |
| `MidiChordState` | Note on/off tracking, Freeze Last Chord, live/frozen masks |
| `TargetNoteGenerator` | Pitch-class mask → octave-expanded target frequencies (≤32) |
| `SubSplitter` | `juce::dsp::LinkwitzRileyFilter` LR4 crossover |
| `TransientDetector` | Fast/slow envelope difference → 0..1 transient envelope |
| `ResonatorBank` / `SvfResonator` | TPT state-variable bandpass voices, glide, drive, stereo detune, `1/√N` normalisation |
| `ColourProcessor` | High-shelf emphasis, saturation, harmonic density, M/S width |
| `PseudoFormantTone` | 3 movable peak biquads + tilt; centre = `base · 2^(st/12)` |
| `SafetyLimiter` | Soft-knee tanh clip guard above 0.98 |
| `AlgoModes` | Per-algorithm tuning table (Clean/Colour/Hyper/HiTECH/Broken) |

## Algorithms are tuning modes, not engines (spec §18.6)

`AlgoModes.h` holds one `AlgoProfile` per algorithm. Each block, the profile is
combined with the live macros to produce `ResonatorBank::Tuning` and
`ColourProcessor::Settings`. HiTECH/Broken mainly shorten Glide
(`glideScale`) and bias Transient up/down; they do not switch DSP paths.

## Audio-thread rules

No allocation, locks, logging or file I/O on the audio thread. Scratch buffers
(`dryBuf`, `lowBuf`, `highBuf`, `dryHighBuf`, `transientEnv`) are sized once in
`prepareToPlay`. Macros are smoothed with `juce::SmoothedValue`; resonator
frequencies are smoothed per block by the Glide one-pole.

## Known simplifications (MVP)

- Resonator coefficients update per block (not per sample); Glide is block-rate.
- The Colour "harmonic density" is a light asymmetric term, not a full exciter.
- `PseudoFormantTone` is a movable-peak tone shaper, **not** a phase-vocoder
  formant shifter (spec §20.3).
- `SpectrumMapView` is diagnostic only; it does not do spectral analysis.
