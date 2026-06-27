#pragma once

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
    alien,
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
        case Character::alien:  return "Alien";
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
    float formantReact;   // how much the character leans on the formant shaper (Alien high)
    float colorResponse;  // overall COLOR sensitivity multiplier
    float air;            // octave-up resonance + air shelf (shine)
    float shimmer;        // grid-oscillator octave shimmer amount
};

inline const CharacterProfile& getCharacterProfile (Character c) noexcept
{
    static constexpr std::array<CharacterProfile, (size_t) Character::count> table {{
        //  qBase qColor tailQ drive hiEmph width glide tBias  form  colResp  air  shimmer
        {   5.0f,  4.0f,  6.0f, 0.05f, 0.20f, 0.25f, 1.20f,  0.10f, 0.4f, 0.85f, 0.20f, 0.15f }, // Clean
        {   9.0f,  8.0f, 16.0f, 0.28f, 0.55f, 0.45f, 1.00f,  0.00f, 0.7f, 1.00f, 0.45f, 0.40f }, // Color
        {  14.0f, 14.0f, 28.0f, 0.50f, 0.90f, 0.70f, 0.90f, -0.05f, 1.0f, 1.25f, 0.85f, 0.80f }, // Hyper
        {  12.0f, 12.0f, 24.0f, 0.45f, 0.70f, 0.60f, 0.70f,  0.00f, 1.6f, 1.15f, 0.70f, 0.65f }, // Alien
        {  11.0f, 10.0f,  9.0f, 0.50f, 0.78f, 0.40f, 0.30f,  0.30f, 0.6f, 1.10f, 0.60f, 0.50f }, // Glitch
    }};

    const auto idx = (size_t) c;
    return table[idx < table.size() ? idx : 1];
}
} // namespace nscm
