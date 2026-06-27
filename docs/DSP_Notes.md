# DSP Notes (v0.5)

All DSP lives in `Source/dsp/` as header-only, UI-free, audio-thread-safe units.
The music-theory and colour-core classes carry no JUCE dependency so they are
tested standalone in `tests/DspSmoke.cpp`.

## Signal flow (spec §12.1)

```
Audio In
 → copy to dryBuf
 → AffectedRange (LR4 highpass @ lowCut + lowpass @ highCut) → activeBuf
       active → dryActive copy (kept) + tunedBuf copy
       active → TransientDetector → 0..1 envelope
 → ColourMappingCore(tunedBuf, dryActive):
       ResonatorBank (grid-tuned SVF) → ColourProcessor (drive/emphasis/width)
       → ENERGY MATCH tuned level to input → blend dry→tuned by amount·COLOR
       → + colourful tail for COLOR>100% → Gate ducks tail when input is quiet
 → PseudoFormantTone (Formant + Gamma)
 → optional Side Mute (collapse processed band to mono)
 → per-sample: transient restore, then  out = dry + Mix·(processed − active)
 → Output gain → SafetyLimiter → Audio Out
```

`out = dry + Mix·(processed − active)` means everything outside [lowCut, highCut]
(including the protected sub) passes through untouched, and Mix only controls how
much of the *transformation* is applied.

## Why it is audible (the key fix)

The v0.3 build summed high-Q bandpass voices normalised by 1/√N — far quieter than
the dry — so the effect was inaudible. `ColourMappingCore` now **energy-matches**
the tuned signal to the input (per-channel envelope followers compute a smoothed
gain so the wet tracks the input's loudness, clamped ≤ 12×). Verified by the
audibility test: broadband noise tuned to A4 concentrates energy ~hundreds× at
440 Hz while output RMS stays ≈ input RMS.

## COLOR 0-200%

- `color01 = min(COLOR, 1)` blends dry → tuned (Chroma "add colour" range).
- `colorBoost = max(COLOR − 1, 0)` raises resonator Q (tail), saturation drive and
  adds an extra resonance layer (PITCHMAP-style "more electronic" range).

## Classes

| Class | Role |
|---|---|
| `ScaleNoteSet` | Key + scale → 12-bit pitch-class mask (12 scales incl. Whole Tone, Chromatic) |
| `MidiChordState` | MIDI note tracking, Freeze last chord |
| `TargetNoteGenerator` | Mask → octave-expanded target frequencies (≤32), Scale Shift |
| `AffectedRange` | LR4 band split for the processed range / protected remainder |
| `TransientDetector` | Fast/slow envelope difference → transient envelope |
| `ResonatorBank` / `SvfResonator` | TPT SVF bandpass voices, glide, drive, stereo detune |
| `ColourProcessor` | High-shelf emphasis, saturation, harmonic density, M/S width |
| `ColourMappingCore` | Resonators + colour + **energy match** + blend + gate (the audible core) |
| `PseudoFormantTone` | Movable peak biquads + tilt (Formant/Gamma), not a real shifter |
| `SafetyLimiter` | Soft-knee tanh clip guard |
| `CharacterModes` | Per-character tuning table (Clean/Color/Hyper/Alien/Glitch) |

## Grid modes

- **Scale** — pitch grid = `ScaleNoteSet(key, scale)`.
- **MIDI** — grid = held chord pitch classes (Freeze keeps the last chord).
- **Hybrid** — union of scale ∪ MIDI (stay in key, emphasise played notes).
- **UI** — MVP uses the scale grid (UI keyboard editing is v1+).

## MVP simplifications

- Resonator coefficients update per block; glide is internal (~30 ms × character).
- Morph / Multirate are present as parameters but light/placeholder (spec v1).
- Gamma feeds the pseudo-formant emphasis; Gate ducks the tail with input level.
- `VisualizerView` is diagnostic only — no real spectral analysis (spec §6.5).
