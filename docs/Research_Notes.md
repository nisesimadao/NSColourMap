# Research Notes

## Colour Bass Workflow Summary

Common approaches:
- Vocoder with MIDI chords
- Resonator / Spectral Resonator tuned to chord notes
- PitchMap-style pitch remapping
- Convolution / cabinet / reverb for tone
- OTT / saturation / multiband after harmonic processing

## HiTECH / Glitch Workflow Summary

Common needs:
- short transients
- laser / fill sounds
- FM / noise / metallic one-shots
- fast automation
- stable low end
- controlled stereo in highs only

## What NSColourMap takes from these workflows

- MIDI chord control from vocoder and PitchMap-style workflows
- Resonator bank as the practical MVP core
- Sub Protect because bass processing often muddies the low end
- Transient Bypass because Colour Bass and HiTECH need attack
- Pseudo Formant Tone for vocal / robot / alien timbre
- HiTECH as a tuning mode for fast, hard, short sounds
- Chroma-style workflow should stay fast and melodic: keep the existing wide
  colour/shimmer by default, then expose a single Melody macro that foregrounds
  input-related grid notes instead of replacing the colour engine.
- Map character research: pitch-grid colour tools are built around separating
  pitched components, moving each component toward a selected scale/MIDI grid,
  preserving transients, and shaping formant/gamma/range. NSColourMap keeps the
  UI generic, but the former Alien slot now uses a fixed 128-pitch-slot layer
  that detects semitone bands and resynthesizes them at nearest grid notes.

## What NSColourMap does **not** do in the MVP

- No true polyphonic pitch correction
- No source separation
- No STFT resynthesis
- No PitchMap UI clone
- No separate HiTECH DSP engine
