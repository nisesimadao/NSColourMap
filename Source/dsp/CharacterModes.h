#pragma once

#include <cstddef>

#include <array>

namespace nscm
{
// Character modes are tuning modes over the same colour-mapping DSP, not separate
// engines (spec v0.5 §10).
enum class Character
{
    clean = 0,
    color,
    hyper,
    alien, // Legacy parameter index; now the pitch-grid mapping character.
    glitch,
    count
};

inline const char* getCharacterName (Character c) noexcept
{
    switch (c)
    {
        case Character::clean:  return "Clean";
        case Character::color:  return "Color";
        case Character::hyper:  return "Hyper";
        case Character::alien:  return "Map";
        case Character::glitch: return "Glitch";
        case Character::count:
        default:                return "Color";
    }
}

// Static character tuning, combined with the live COLOR / Amount macros.
struct CharacterProfile
{
    float qBase;          // resonator Q at COLOR 0
    float qColor;         // extra Q as COLOR goes 0->100%
    float tailQ;          // extra Q as COLOR goes 100->200% (ringing tail)
    float drive;          // colour-stage saturation
    float highEmph;       // high-harmonic emphasis
    float width;          // stereo spread
    float glideScale;     // internal glide multiplier (Glitch = short)
    float transientBias;  // added to the Transient macro
    float formantReact;   // how much the character leans on the formant shaper
    float colorResponse;  // overall COLOR sensitivity multiplier
    float air;            // octave-up resonance + air shelf (shine)
    float shimmer;        // grid-oscillator octave shimmer amount
    float mapFocus;       // foreground input-related grid pitches
    float mapPurity;      // reduce extra harmonics/shimmer for a cleaner pitch-grid lock
};

inline const CharacterProfile& getCharacterProfile (Character c) noexcept
{
    static constexpr std::array<CharacterProfile, (std::size_t) Character::count> table {{
        //  qBase qColor tailQ drive hiEmph width glide tBias  form  colResp  air  shimmer mapF mapP
        {   5.5f,  4.5f,  7.0f, 0.06f, 0.24f, 0.30f, 1.20f,  0.10f, 0.4f, 0.90f, 0.24f, 0.18f, 0.00f, 0.10f }, // Clean
        {   9.5f,  8.8f, 17.5f, 0.31f, 0.60f, 0.50f, 1.00f,  0.00f, 0.7f, 1.06f, 0.52f, 0.46f, 0.00f, 0.20f }, // Color
        {  14.5f, 13.0f, 27.0f, 0.47f, 0.86f, 0.74f, 0.90f, -0.06f, 1.0f, 1.22f, 0.78f, 0.72f, 0.16f, 0.10f }, // Hyper
        {  18.0f, 18.5f, 34.0f, 0.36f, 0.66f, 0.58f, 0.55f, -0.10f, 0.9f, 1.24f, 0.62f, 0.45f, 0.82f, 0.78f }, // Map
        {  12.0f, 11.5f, 11.0f, 0.46f, 0.82f, 0.50f, 0.25f,  0.28f, 0.6f, 1.12f, 0.64f, 0.56f, 0.18f, 0.05f }, // Glitch
    }};

    const auto idx = (std::size_t) c;
    return table[idx < table.size() ? idx : 1];
}
} // namespace nscm
