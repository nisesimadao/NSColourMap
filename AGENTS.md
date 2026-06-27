# AGENTS.md

## Project Goal
Build NSColourMap, a JUCE/CMake VST3/AU plugin for Colour Bass and HiTECH sound design.

## Core Idea
NSColourMap's MVP is **not** a true polyphonic pitch-correction engine.
It is a MIDI / scale-aware harmonic **Resonator Bank** plus a **Colour Processor**,
with **Sub Protect**, **Transient Bypass** and a **Pseudo Formant Tone** shaper.
The goal is the practical Colour Bass feeling of PitchMap / Vocoder / Resonator
workflows — not to recreate PitchMap internally.

## MVP Definition
The MVP is a MIDI / scale-aware resonator effect. It should prove this use case:
1. Insert NSColourMap on a bass / noise / vocal-chop track.
2. Send MIDI chords to the plugin.
3. Hear the resonator colour change according to the chord.
4. Keep the sub stable with Sub Protect.
5. Blend the result with Mix.

If this use case does not work yet, do not implement advanced UI, STFT, custom
maps, or extra presets.

## Architecture (current)
- `Source/Parameters.*` — APVTS parameter layout (single source of truth for IDs).
- `Source/PluginProcessor.*` — signal flow + UI state atomics.
- `Source/PluginEditor.*` — LookAndFeel, SpectrumMapView, KeyboardView, controls.
- `Source/dsp/*.h` — header-only, UI-free DSP classes:
  `ScaleNoteSet`, `MidiChordState`, `TargetNoteGenerator`, `SubSplitter`,
  `TransientDetector`, `ResonatorBank`, `ColourProcessor`, `PseudoFormantTone`,
  `SafetyLimiter`, `AlgoModes`.
- `tests/DspSmoke.cpp` — header-only theory/target tests (no JUCE).

## Rules
- Keep every change small and buildable; always keep a working build.
- Do not implement full PitchMap-style source separation in the MVP.
- Do not implement STFT resynthesis unless explicitly requested in a later version.
- Do not rewrite Japanese documentation unless asked. Preserve UTF-8.
- Prefer simple, testable DSP classes. Every DSP class is usable without the UI.
- Audio thread must not allocate, lock, log, or perform file I/O.
- UI and DSP must be separated. No hidden global state for audio processing.
- Do not use absolute layout coordinates for the UI.
- Do not create giant files. Split DSP/UI/parameter code into small classes.
- `SpectrumMapView` is diagnostic / visual only in the MVP.
- HiTECH is an algorithm tuning mode, not a separate DSP engine.
- The Formant control is a pseudo-formant tone shaper, not a true formant shifter.
