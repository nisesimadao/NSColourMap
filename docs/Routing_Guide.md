# MIDI Routing Guide

NSColourMap is an audio effect that can receive MIDI.
It is not a MIDI effect or an instrument, so chord MIDI must be routed to the audio track or plugin instance that hosts NSColourMap.
The exact routing method depends on the DAW.

If the **MIDI LED** in the plugin header does not light, the plugin is not receiving MIDI.
You can either fix the routing or use **Grid Mode → Scale**, which does not require MIDI input.

## Ableton Live

1. Insert NSColourMap on the audio track that contains the bass, noise, or other source material.
2. Create a MIDI track containing the chord notes.
3. On the MIDI track, set **MIDI To → <audio track>** and select **NSColourMap** in the second destination field.
4. Set the MIDI track's **Monitor** to **In**, or arm the track, so MIDI is forwarded.
5. Confirm that the MIDI LED in NSColourMap lights when notes are sent.

## FL Studio

1. Add NSColourMap to the target audio path, for example on a Mixer insert or inside Patcher.
2. Route a MIDI source to the plugin using MIDI Out / Patcher, or assign a MIDI input port in the plugin wrapper and send a Piano Roll channel to the same port.
3. Confirm that the MIDI LED lights when notes are sent.

## REAPER

1. Insert NSColourMap on the audio track.
2. Either place MIDI items on the same track, or route a second MIDI track into the audio track.
3. Make sure MIDI monitoring and routing are enabled for the path you chose.
4. Confirm that the MIDI LED lights when notes are sent.

## Logic Pro (AU)

Logic does not provide the same direct MIDI-to-audio-effect routing path as some other DAWs.
The simplest option is **Grid Mode → Scale**, which requires no MIDI.

If your session provides a MIDI-capable routing path to the AU effect, you can use that instead.
An instrument-compatible build is listed as a future option in spec §8.2.

## Without MIDI: Scale mode

1. Set **Grid Mode → Scale**.
2. Choose **Key** and **Scale**.
3. Use **Scale Shift** to move the target grid in semitone steps.

Minor Pentatonic, Pentatonic Blues, and Natural Minor are useful starting points for Colour Bass material, but the appropriate scale depends on the track.
