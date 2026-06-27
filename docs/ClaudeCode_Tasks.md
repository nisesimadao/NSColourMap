# ClaudeCode Task Log

Implementation status against the phases in `NSColourMap_Spec.md` (§25).

## Done (MVP)

- **Phase 0 — Repository setup**: CMake project `NSColourMap`, VST3 + AU, MIDI input
  enabled, JUCE 8.0.14 via FetchContent. Builds with an empty-to-full UI.
- **Phase 1 — Parameters + state**: `Source/Parameters.*` APVTS layout for mode,
  algo, amount, glide, colour, formant, subProtect, transient, mix, output,
  freezeChord, key, scale, scaleShift, quality, uiTab. Save/restore via XML.
- **Phase 2 — MIDI foundation**: `MidiChordState` (note on/off, lowest note,
  Freeze Last Chord, live/frozen masks). Header LED + active-chord text.
- **Phase 3 — Scale Resonance foundation**: `ScaleNoteSet` (10 scales) +
  `TargetNoteGenerator` (octave-expanded targets, Scale Shift). Mode switch.
- **Phase 4 — Sub split + transient bypass**: `SubSplitter` (LR4) +
  `TransientDetector`; protected sub stays dry, transients pass through.
- **Phase 5 — Resonator Bank**: `ResonatorBank` of TPT SVF bandpass voices with
  glide, drive, stereo detune and `1/√N` normalisation; quality = voice count.
- **Phase 6 — UI MVP 1**: header, MIDI LED, mode/algo selectors, macro knobs,
  responsive layout, preserved NSDucking look & feel.
- **Phase 7 — Colour + Pseudo Formant + Algo tuning**: `ColourProcessor`,
  `PseudoFormantTone`, `AlgoModes` (Clean/Colour/Hyper/HiTECH/Broken as tuning).
- **Phase 8 (partial) — UI MVP 2/3**: `KeyboardView` (target + held highlights),
  `SpectrumMapView` (diagnostic pitch lanes, sub zone, transient flash, energy).

## Tests

- `tests/DspSmoke.cpp` — header-only checks for `ScaleNoteSet`, `MidiChordState`,
  `TargetNoteGenerator` (relative-minor equality, freeze behaviour, A440, ordering).
  Run via `ctest --test-dir build`.

## Done (v1)

- **Phase 9 — Presets**: 10 factory presets (`Source/Presets.h`, spec §23 / §27)
  selectable from the header preset menu; applied via APVTS.
- **Snapshot A/B/C/D**: full-state store / recall (`Source/PluginProcessor` snapshot
  API). Click stores empty / recalls filled, Alt-click overwrites. Persisted in the
  plugin state via a wrapped `NSColourMapState` XML (backward compatible).

## Next (v1+)

- Auto gain, advanced parameter UI (Resonance/Decay/Drive/Width), CPU quality polish.
- Instrument-compatible build for DAWs that can't route MIDI into FX (spec §8.2).
- v2: STFT pitch-class mapper, custom map mode, convolution IR, glitch sequencer.
