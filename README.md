# NSColourMap

**NSColourMap** is a free JUCE-based **VST3 / AU** effect for **Colour Bass** and **HiTECH** sound design.

It turns basses, noise, vocal chops and metallic textures into **MIDI / scale-aware
harmonic colour textures** using a resonator bank, sub protection, transient bypass
and a simplified formant / tone shaper.

> It is **not** a true PitchMap clone or a polyphonic pitch-correction engine.
> It is a MIDI / scale-aware **Resonator Bank + Colour Processor**.

---

## What it does

Send MIDI chords (or pick a Key + Scale) and NSColourMap resonates your audio
toward those notes, adding colour-bass harmonics while keeping the low end clean.

```
Audio In → Sub Split → [ Transient detect ] → Resonator Bank → Colour → Pseudo Formant
         → Transient return → Dry/Wet Mix → Output → Safety limiter → Audio Out
                ▲
            MIDI Chord / Scale Resonance targets
```

## Controls

| Control | Range | What it does |
|---|---|---|
| **Mode** | MIDI Chord / Scale Resonance | Where target notes come from |
| **Algo** | Clean / Colour / Hyper / HiTECH / Broken | Character tuning (one DSP, five voicings) |
| **Amount** | 0–100% | Depth of the colour effect |
| **Glide** | 0–500 ms | Smooths resonator pitch when the chord changes |
| **Colour** | 0–100% | Harmonics, brightness, density, width |
| **Formant** | −24…+24 st | Pseudo-formant tone (big/dark ↔ small/bright) |
| **Sub Protect** | 40–200 Hz | Low band kept dry & mono-safe |
| **Transient** | 0–150% | Lets attacks pass through dry |
| **Mix** | 0–100% | Dry / wet |
| **Output** | −12…+12 dB | Output gain |
| **Key / Scale / Scale Shift** | — | Scale Resonance Mode targets |
| **Freeze** | On / Off | Holds the last chord after note-off |
| **Quality** | Eco / Normal / High | 12 / 24 / 32 resonator voices |

## MIDI routing

NSColourMap is an **audio effect that receives MIDI** (not a MIDI effect / not an
instrument). Routing MIDI into an audio effect differs per DAW. If the plugin
doesn't react to your chords, check the **MIDI LED** in the header — if it's dark,
MIDI isn't arriving, so use **Scale Resonance Mode** instead.

See [`docs/Routing_Guide.md`](docs/Routing_Guide.md) for Ableton Live, FL Studio,
REAPER and Logic Pro examples.

## Safety / tips

- For bass-heavy material keep **Sub Protect** around 100–150 Hz.
- For full basses use **Mix** around 40–70% and keep a separate clean sub if needed.
- HiTECH and Broken work best on short noise / FM / metallic one-shots.

---

## Build

Requires CMake ≥ 3.22 and a C++17 compiler. JUCE 8.0.14 is fetched automatically.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build            # DSP smoke tests
```

Artefacts: `build/NSColourMap_artefacts/Release/{VST3,AU}/`.

## Status

MVP per [`docs/NSColourMap_Spec.md`](docs/NSColourMap_Spec.md). Presets, snapshot
A/B/C/D, advanced parameter UI and STFT mapping are planned for later versions.

## License

MIT · by nisesimadao
