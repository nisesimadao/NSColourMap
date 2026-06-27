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

## What NSColourMap does **not** do in the MVP

- No true polyphonic pitch correction
- No source separation
- No STFT resynthesis
- No PitchMap UI clone
- No separate HiTECH DSP engine
