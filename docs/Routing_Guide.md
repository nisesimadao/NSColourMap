# MIDI Routing Guide

NSColourMap is an **audio effect that accepts MIDI input**. It is not a MIDI
effect and not an instrument, so the chord MIDI has to be routed *into* the audio
track that hosts the plugin. Each DAW does this differently.

If the **MIDI LED** in the plugin header never lights up, MIDI is not reaching the
plugin — fall back to **Scale Grid mode** (set Key + Scale) which needs no MIDI.

## Ableton Live
1. Put NSColourMap on the **audio track** with your bass/noise.
2. Create a **MIDI track** with your chords.
3. On the MIDI track, set **MIDI To → (the audio track)** and the second selector
   to **NSColourMap**.
4. Set the MIDI track **Monitor** to **In** (or arm it) so MIDI flows through.

## FL Studio
1. Add NSColourMap to the audio track (or a mixer insert via Patcher).
2. Use a **MIDI Out** / Patcher to send chords to the plugin, or open the plugin
   wrapper's **MIDI input port** and route a piano-roll channel to that port.
3. Confirm with the MIDI LED.

## REAPER
1. Insert NSColourMap on the audio track.
2. Add a second track with MIDI chords and route its output to the audio track,
   or put the MIDI items **on the same track** as the plugin.
3. Make sure the FX receives MIDI (track has "Record/Monitor MIDI" or the item is
   on the same track).

## Logic Pro (AU)
Logic does not route MIDI into audio-effect plugins easily. Options:
- Use **Scale Grid mode** (no MIDI needed), or
- Place NSColourMap where MIDI is available (e.g. via an instrument-style routing
  / external MIDI environment), or
- Use a future **Instrument-compatible build** (planned, spec §8.2).

## No MIDI? Use Scale Grid mode
1. Set **Grid Mode → Scale**.
2. Pick **Key** and **Scale** (Minor Pentatonic / Pentatonic Blues / Natural Minor
   are good for Colour Bass).
3. Use **Scale Shift** to move the resonance up/down by semitones.
