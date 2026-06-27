# ClaudeCode Task Log (v0.5 redesign)

Status against the phases in `NSColourMap_Spec.md` (v0.5 §18).

## Done (MVP)

- **Phase 0 — Legacy-free setup**: dropped the resonator-only framing; kept Sub
  protection (Low Cut), Transient, MIDI LED. New AGENTS/README/docs.
- **Phase 1 — Minimal plugin**: VST3 + AU audio effect, MIDI input, stereo, APVTS.
- **Phase 2 — Key / Scale / MIDI grid**: `ScaleNoteSet` (12 scales),
  `MidiChordState`, `TargetNoteGenerator`, Scale Shift, Freeze, MIDI LED.
- **Phase 3 — Affected range + safety**: `AffectedRange` (LR4 Low/High Cut),
  protected remainder, `SafetyLimiter`, output gain.
- **Phase 4 — Colour Mapping Core**: `ColourMappingCore` — grid-tuned resonators,
  **energy-matched to input** (audible), COLOR 0-200% (dry/wet + resonance/tail),
  Amount / Mix. This is the headline fix vs the inaudible v0.3 build.
- **Phase 5 — Visualizer**: `VisualizerView` shows DRY / TUNED / COLORED, target
  lanes, affected range, protected zones, MIDI markers, transient flash.
- **Phase 6 — Character modes**: Clean / Color / Hyper / Alien / Glitch
  (`CharacterModes`, tuning over the same DSP).
- **Phase 7 — Pseudo formant + transient**: `PseudoFormantTone` (Formant + Gamma),
  `TransientDetector` restore.

## Tests

- `tests/DspSmoke.cpp` — `ScaleNoteSet` / `MidiChordState` / `TargetNoteGenerator`
  checks **plus a colour-core audibility test**: broadband noise tuned to A4 must
  stay as loud as the input and concentrate energy onto 440 Hz (stable SVF-bandpass
  measurement). Run via `ctest --test-dir build`. AU passes `auval -v aumf Nscm Nscm`.

## Partial / placeholder

- **Phase 8 (Chroma advanced)**: Gate functional; Gamma feeds the formant shaper;
  Side Mute collapses the processed band to mono; Morph / Multirate are parameters
  with light/placeholder behaviour. Custom scales JSON: not yet.

## Next

- Phase 8 proper: spectral-ish Morph, true Multirate, Side-band mute, custom scales.
- v1: UI keyboard click-to-set-root / temporary grid; affected-range drag on the
  visualizer; presets expansion.
- Phase 9 / v2: STFT/CQT pitch-class mapper, magnitude morph, spectral gate per bin,
  custom grid editor.
