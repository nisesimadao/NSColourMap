# AGENTS.md

## Project Goal
Build NSColourMap: a Colour Bass focused VST inspired by PITCHMAP::COLORS-style
pitch-grid colour mapping and Chroma-style fast workflow (spec v0.5).

## Core UX
The plugin should feel like:
- choose a key/scale or send MIDI chords
- turn COLOR
- instantly hear colourful, in-key harmonic transformation

## Important
This is **not** the old resonator-only NSColourMap spec. Do not preserve legacy
architecture unless it supports the COLORS + Chroma direction. The headline
feature is an **audible** colour transform — the v0.3 resonator-only build was
inaudible in a DAW, so the colour core energy-matches the tuned signal to the
input to guarantee presence (see `dsp/ColourMappingCore.h`).

## MVP
The MVP must prove:
1. Key/Scale changes the harmonic colour.
2. MIDI chords change the pitch grid.
3. COLOR 0-200% changes dry/wet and adds resonance/tail.
4. Visualizer shows DRY / TUNED / COLORED feedback.
5. Low-end protection (Low Cut) prevents bass mud.

## Architecture
- `Source/Parameters.*` — APVTS layout (single source of truth for IDs).
- `Source/PluginProcessor.*` — signal flow + UI state atomics.
- `Source/PluginEditor.*` — LookAndFeel, VisualizerView, KeyboardView, Big COLOR.
- `Source/dsp/*.h` — header-only, UI-free DSP:
  `ScaleNoteSet`, `MidiChordState`, `TargetNoteGenerator`, `AffectedRange`,
  `TransientDetector`, `ResonatorBank`, `ColourProcessor`, `ColourMappingCore`,
  `PseudoFormantTone`, `SafetyLimiter`, `CharacterModes`.
- `tests/DspSmoke.cpp` — theory tests + a **colour-core audibility test** (proves
  broadband input concentrates energy onto the grid and stays as loud as the dry).

## Do Not
- Do not copy UI assets, names, or branding from commercial plugins.
- Do not implement full PITCHMAP/COLORS internally in the MVP.
- Do not build a huge UI before the audio core works.
- Do not keep old terminology like "Scale Resonance" as the main UX label.
- Do not make a character mode a separate DSP engine.
- Do not block the build on advanced spectral code.

## Implementation Rules
- Keep changes small and buildable. Preserve UTF-8.
- Audio thread must not allocate, lock, log, or do file I/O.
- UI and DSP must be separated. Every DSP class is testable without the UI.
- Prefer simple MVP approximations behind stable interfaces.
- Make the effect obviously audible and verify it empirically.
