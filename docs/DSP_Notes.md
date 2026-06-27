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

## Why it is audible (two key fixes)

1. **Grid oscillator layer.** A resonator bank can only *emphasise frequencies
   already present* in the input, so on off-key or inharmonic material it does
   almost nothing — which is why earlier builds sounded like "nothing changed."
   `ColourMappingCore` now drives a bank of **sine+harmonic oscillators tuned to
   the grid notes** with the input's amplitude envelope, *synthesising* an in-key
   chord from any source (noise, off-key bass, chops). The resonator bank remains
   as a small emphasis/texture layer. The oscillator layer is what makes the
   colour obvious and genuinely tonal.
2. **Energy match.** The combined tuned signal is scaled (per-channel envelope
   followers, smoothed gain, slight +presence) to track the input's loudness, so
   the colour is always as loud as the dry.

Earlier there was also a real bug: the COLOR / Amount / Formant / Gamma / Gate
smoothers were read with `getCurrentValue()` but never advanced, so those knobs
were frozen at their load-time values. They are now advanced with `skip()` each
block.

## Shine / brilliance

Brightness comes from three cooperating layers, all scaled by COLOR and the
Character profile's `air` / `shimmer`:
- **Resonator air** — each `ResonatorVoice` adds an octave-up, higher-Q bandpass
  (`leftHi/rightHi`) with a few cents of L/R detune → bell-like sparkle.
- **Oscillator shimmer** — each grid oscillator adds a detuned octave-up partial
  whose detune is modulated by a slow ~0.6 Hz LFO → a living, beating shimmer.
  Harmonics and the shimmer partial are band-limited per voice (skip any partial
  above 0.45·fs) to avoid aliasing on high grid notes.
- **Air shelf** — `ColourProcessor` brightens content above ~3.8 kHz.

Character brightness ranking: Hyper > Alien > Glitch > Color > Clean (measured:
Hyper at COLOR 200% is ~4.7× brighter above 3.5 kHz than the dry input).

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
| `ColourMappingCore` | **Grid oscillators** (synthesise in-key tones) + resonator emphasis + colour + **energy match** + blend + gate (the audible core) |
| `PseudoFormantTone` | Movable peak biquads + tilt (Formant/Gamma), not a real shifter |
| `SafetyLimiter` | Soft-knee tanh clip guard |
| `CharacterModes` | Per-character tuning table (Clean/Color/Hyper/Alien/Glitch) |

## Grid modes

- **Scale** — pitch grid = `ScaleNoteSet(key, scale)`.
- **MIDI** — grid = held chord pitch classes (Freeze keeps the last chord).
- **Hybrid** — union of scale ∪ MIDI (stay in key, emphasise played notes).
- **UI** — MVP uses the scale grid (UI keyboard editing is v1+).

In MIDI mode, releasing all notes (with Freeze off, the default) clears the grid;
a `colourGain` smoother (~80 ms) fades the colour out and the processor then
idles (passes the dry signal through) so nothing keeps sounding. Scale/Hybrid
modes always have a grid, so they stay active by design.

## MVP simplifications

- Resonator coefficients update per block; glide is internal (~30 ms × character).
- Morph / Multirate are present as parameters but light/placeholder (spec v1).
- Gamma feeds the pseudo-formant emphasis; Gate ducks the tail with input level.
- `VisualizerView` is diagnostic only — no real spectral analysis (spec §6.5).
