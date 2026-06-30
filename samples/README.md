# NSColourMap Audio Examples

These files are README examples for the NSColourMap colour-mapping sound.

Use raw GitHub URLs for README playback links. Relative `.wav` links open the
repository blob view on GitHub, which can look like binary text instead of an
audio player.

- `source/bigsoundbank_0836_urinal_flush_water_cc0.wav`
  - Source: BigSoundBank #0836 "Urinal flush water"
  - URL: https://bigsoundbank.com/urinal-flush-water-s0836.html
  - License: CC0 / public domain equivalent
- `toilet_flush_dry_cc0.wav`
  - Short normalized excerpt from the source file.
- `toilet_flush_clean_cmin7.wav`
- `toilet_flush_color_cmin7.wav`
- `toilet_flush_hyper_cmin7.wav`
- `toilet_flush_map_cmin7.wav`
- `toilet_flush_glitch_cmin7.wav`
  - The same excerpt rendered through NSColourMap with preset `10 COLOR 150 Tail`
    as a base, MIDI Grid, and a held Cmin7 chord (`C Eb G Bb`).
- `toilet_flush_nscolourmap_color150_cmin7.wav`
  - Compatibility copy of the Hyper render used by the original before/after
    README example.

Regenerate:

```sh
cmake --build build --target NSColourMap_RenderAudioSamples
./build/NSColourMap_RenderAudioSamples
```
