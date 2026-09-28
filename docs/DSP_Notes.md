# DSP Notes (v0.5)

The DSP implementation lives under `Source/dsp/` as header-only, UI-independent units designed for audio-thread use.
The music-theory and colour-core classes do not depend on JUCE, which allows them to be tested independently in `tests/DspSmoke.cpp`.

## Signal flow (spec §12.1)

```text
Audio In
 → copy to dryBuf
 → AffectedRange (LR4 highpass @ lowCut + lowpass @ highCut) → activeBuf
       active → dryActive copy (kept) + tunedBuf copy
       active → TransientDetector → 0..1 envelope
 → ColourMappingCore(tunedBuf, dryActive):
       ResonatorBank (grid-tuned SVF) → ColourProcessor (drive/emphasis/width)
       → energy-match tuned level to input → blend dry→tuned by amount·COLOR
       → add tail for COLOR > 100% → Gate ducks tail when input is quiet
 → PseudoFormantTone (Formant + Gamma)
 → optional Side Mute (collapse processed band to mono)
 → per-sample transient restore, then out = dry + Mix·(processed − active)
 → Output gain → SafetyLimiter → Audio Out
```

`out = dry + Mix·(processed − active)` leaves content outside `[lowCut, highCut]` unchanged, including the protected sub range.
`Mix` therefore controls the amount of the transformation rather than scaling the full signal path.

## Audibility fixes

Two changes made the colour layer more consistent across source material.

1. **Grid oscillator layer.**
   A resonator bank mainly emphasizes frequencies already present in the input.
   Earlier builds therefore produced little change on strongly off-grid or inharmonic material.
   `ColourMappingCore` now drives sine-plus-harmonic oscillators tuned to the target grid and modulates them from the input amplitude envelope.
   The resonator bank remains as an additional emphasis and texture layer.
2. **Energy matching.**
   Per-channel envelope followers estimate the input and tuned levels.
   A smoothed gain stage keeps the tuned layer near the input's working level so changes in COLOR are easier to compare without large level jumps.

An earlier implementation bug also prevented the COLOR, Amount, Formant, Gamma, and Gate smoothers from advancing.
They were read with `getCurrentValue()` without stepping the smoother.
The current implementation advances them with `skip()` each block.

## High-frequency layers

Brightness comes from three components that are scaled by COLOR and the Character profile's `air` / `shimmer` values.

- **Resonator air**: each `ResonatorVoice` adds an octave-up, higher-Q band-pass component with slight left/right detuning.
- **Oscillator shimmer**: each grid oscillator adds a detuned octave-up partial whose detune is modulated by a slow ~0.6 Hz LFO.
  Harmonics above `0.45 * fs` are skipped to limit aliasing.
- **Air shelf**: `ColourProcessor` adds high-frequency emphasis above approximately 3.8 kHz.

In the current measurement setup, the Character brightness ordering is Hyper > Map > Glitch > Color > Clean.
For the test signal used during development, Hyper at COLOR 200% measured about 4.7× more energy above 3.5 kHz than the dry input.
This is a test result, not a fixed response guarantee for arbitrary material.

## COLOR 0–200%

- `color01 = min(COLOR, 1)` controls the dry-to-tuned blend over the 0–100% range.
- `colorBoost = max(COLOR − 1, 0)` raises resonator Q, saturation drive, and the additional resonance layer over the 100–200% range.

## Classes

| Class | Role |
|---|---|
| `ScaleNoteSet` | Key + scale → 12-bit pitch-class mask, including Whole Tone and Chromatic |
| `MidiChordState` | MIDI note tracking and Freeze state |
| `TargetNoteGenerator` | Expands pitch classes across octaves, up to 32 targets, and applies Scale Shift |
| `AffectedRange` | LR4 split for the processed frequency range and protected remainder |
| `TransientDetector` | Fast/slow envelope difference used as the transient envelope |
| `ResonatorBank` / `SvfResonator` | TPT SVF band-pass voices with glide, drive, and stereo detune |
| `ColourProcessor` | High-shelf emphasis, saturation, harmonic density, and M/S width |
| `ColourMappingCore` | Grid oscillators, resonator emphasis, energy matching, dry/tuned blend, tail, and gate |
| `PseudoFormantTone` | Movable peak filters and tilt; this is a formant-like shaper, not a true formant shifter |
| `SafetyLimiter` | Soft-knee tanh output guard |
| `CharacterModes` | Per-character tuning table for Clean / Color / Hyper / Map / Glitch |

## Grid modes

- **Scale**: grid = `ScaleNoteSet(key, scale)`.
- **MIDI**: grid = held MIDI pitch classes; Freeze keeps the most recent chord.
- **Hybrid**: union of Scale and MIDI pitch classes.
- **UI**: currently uses the scale grid; direct UI-keyboard editing is planned for a later version.

In MIDI mode, releasing all notes with Freeze disabled clears the grid.
A `colourGain` smoother of about 80 ms fades the colour layer out before the processor returns to dry passthrough.
Scale and Hybrid modes always retain a grid by design.

## Gamma, Morph, and de-harsh processing

- **Gamma** controls `PseudoFormantTone`.
  The shaper uses three peaks between approximate `ah` formants (700 / 1220 / 2600 Hz) and `ee` formants (350 / 2000 / 2900 Hz), plus anti-resonance notches between them.
  A slow ~0.3 Hz morph moves between the shapes, while Formant applies a `2^(st/12)` frequency ratio.
- **Morph** transfers the dry signal's fast amplitude contour, approximately 2 ms, to the processed signal so attacks are retained while sustained tails can be reduced.
- **De-harsh** applies dynamic compression to the high-frequency air band rather than a fixed cut.
- **Level control** limits the processed envelope to roughly 1.2× the input envelope in the current implementation.

## High Quality: STFT spectral snap (spec §12.3 / Phase 9)

`Quality = High Quality` uses `SpectralMapper`, a 2048-point Hann overlap-add FFT with 75% overlap.
Within the active band, bins that contain target scale notes are retained or emphasized and other bins are attenuated.
The implementation checks whether a bin contains a scale note instead of assigning every low-frequency bin to one rounded pitch class, which avoids over-attenuating coarse bass bins.

This mode adds `fftSize` samples of latency.
The processor reports that latency and delays the dry and original-active paths with `DelayLine` so recombination remains aligned.
`0 Latency` uses the oscillator/resonator path without that STFT delay.

In the development test signal, the measured in-grid/off-grid energy ratio was higher in High Quality mode than in the 0-Latency path.

## Latency reporting

Latency is set from the selected Quality during `prepareToPlay` because hosts may query latency before audio processing begins, including during offline export.
Updating it only from `processBlock` can leave export PDC with a stale value.

The internal dry delay is set to match the STFT path.
Impulse tests used during development measured 0 samples for 0 Latency mode and 2048 samples for the current High Quality configuration, matching the reported values.

## Spectrum analyzer

`SpectrumAnalyzer` runs a 2048-point magnitude FFT on the output with 75% overlap.
It writes 128 log-spaced, peak-smoothed bins to atomics for the UI thread.
`VisualizerView` reads those values and draws the spectrum behind the pitch lanes.

## Current simplifications

- Resonator coefficients update once per block; per-character glide is handled internally and is approximately 30 ms.
- Multirate is currently a parameter placeholder.
- Gate controls the tail from input level.
