# NSColourMap

**NSColourMap** is a free JUCE-based **VST3 / AU** effect for **Colour Bass** sound design.

It fuses **PITCHMAP::COLORS**-style pitch-grid colour transformation with
**Chroma**-style "drop it in, pick a Key/Scale, turn COLOR" simplicity. Bass,
noise, FM, wobbles, vocal chops and metallic textures get pulled toward a musical
pitch grid and turn into colourful, in-key harmonic textures.

> Not a pitch-correction / Auto-Tune tool. It is a spectral **colour mapper**.

---

## Quick start

1. Drop NSColourMap on your sound-design track.
2. Pick a **Key / Scale**, or send **MIDI chords** (set Grid Mode to MIDI).
3. Turn **COLOR** up.
4. Use **Mode** and **Scale Shift** to move the colour.
5. Blend with **Mix**; protect the low end with **Low Cut**.

The big **COLOR** knob is all you need to start — everything else is fine-tuning.

## Controls

| Control | Range | What it does |
|---|---|---|
| **Grid Mode** | Scale / MIDI / Hybrid / UI | Where the pitch grid comes from |
| **Character** | Clean / Color / Hyper / Alien / Glitch | Overall flavour (one DSP, five tunings) |
| **COLOR** | 0–200% | Main macro. 0–100% dry→tuned; 100–200% adds resonance + colourful tail |
| **Amount** | 0–100% | How hard the audio is pushed onto the grid |
| **Scale Shift** | −12…+12 st | Moves the whole pitch grid (automate it!) |
| **Formant** | −24…+24 st | Pseudo-formant size / voice character |
| **Transient** | 0–150% | Lets attacks pass through dry |
| **Mix / Output** | — | Dry/wet and output gain |
| **Key / Scale** | — | Scale Grid target notes (12 scales incl. Whole Tone, Chromatic) |
| **Freeze** | On/Off (default Off) | Off: the colour stops when you release the notes. On: holds the last chord |
| **Quality** | 0 Latency / High Quality | Voice count / smoothing |
| **Advanced** | — | Gamma · Morph · Gate · Low Cut · High Cut · Side Mute · Multirate |

## Visualizer

The central display shows what the colour engine is doing:

- **DRY** (purple) — the original energy
- **TUNED** (cyan) — energy pulled onto the pitch-grid lanes
- **COLORED** (amber) — extra resonance/tail added above COLOR 100%
- **PROTECTED** (grey) — outside the affected range (Low Cut / High Cut)

## MIDI routing

NSColourMap is an audio effect that **receives** MIDI. If the **MIDI LED** in the
header stays dark, MIDI isn't reaching it — switch **Grid Mode → Scale** (no MIDI
needed) or fix routing. See [`docs/Routing_Guide.md`](docs/Routing_Guide.md).

If it doesn't react: 1) check the MIDI LED, 2) enable MIDI Grid mode, 3) turn
Freeze on, 4) try Scale Grid first, 5) raise COLOR and Mix.

---

## Build

Requires CMake ≥ 3.22 and a C++17 compiler. JUCE 8.0.14 is fetched automatically.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build       # theory + colour-core audibility tests
```

Artefacts: `build/NSColourMap_artefacts/Release/{VST3,AU}/`.

## Status

MVP per [`docs/NSColourMap_Spec.md`](docs/NSColourMap_Spec.md) (v0.5: PITCHMAP::COLORS ×
Chroma chimera). Morph / Side Mute / Multirate are present as light placeholders;
Gamma feeds the formant shaper; Gate is functional. STFT pitch-class mapping is v2.

## License

MIT · by nisesimadao
