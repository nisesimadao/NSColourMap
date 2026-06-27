#pragma once

#include <array>

namespace nscm
{
// The five algorithms are tuning modes over the SAME DSP, not separate engines
// (spec §5.4 / §18.6 / §21).
enum class Algo
{
    clean = 0,
    colour,
    hyper,
    hitech,
    broken,
    count
};

inline const char* getAlgoName (Algo a) noexcept
{
    switch (a)
    {
        case Algo::clean:  return "Clean";
        case Algo::colour: return "Colour";
        case Algo::hyper:  return "Hyper";
        case Algo::hitech: return "HiTECH";
        case Algo::broken: return "Broken";
        case Algo::count:
        default:           return "Colour";
    }
}

// Static per-algo character. Combined with the live macros to produce the
// resonator + colour settings each block.
struct AlgoProfile
{
    float qBase;
    float qColour;       // extra Q from the Colour macro
    float driveBase;     // resonator/colour saturation
    float stereoCents;   // L/R detune
    float wetScale;      // base gain into the resonator bank
    float highEmph;      // colour processor high emphasis
    float widthScale;    // stereo width
    float glideScale;    // multiplier on the Glide macro (HiTECH/Broken = short)
    float formantScale;  // how strongly the algo leans on the formant shaper
    float transientBias; // added to the Transient macro
    float chaos;         // Broken-only random detune amount
};

inline const AlgoProfile& getAlgoProfile (Algo a) noexcept
{
    static constexpr std::array<AlgoProfile, (size_t) Algo::count> table {{
        //  qBase qCol drive stCents wet  hiEmph width glide form  tBias chaos
        {   5.0f, 4.0f, 0.00f,  3.0f, 0.60f, 0.20f, 0.20f, 1.20f, 0.6f,  0.10f, 0.00f }, // Clean
        {   8.0f, 8.0f, 0.25f,  6.0f, 0.90f, 0.50f, 0.40f, 1.00f, 0.9f,  0.00f, 0.00f }, // Colour
        {  13.0f,12.0f, 0.45f, 12.0f, 1.10f, 0.85f, 0.70f, 0.85f, 1.1f, -0.10f, 0.00f }, // Hyper
        {  11.0f, 9.0f, 0.40f,  8.0f, 1.00f, 0.75f, 0.50f, 0.40f, 1.0f,  0.25f, 0.00f }, // HiTECH
        {  16.0f,10.0f, 0.60f, 10.0f, 1.10f, 0.80f, 0.60f, 0.20f, 1.0f, -0.30f, 0.18f }, // Broken
    }};

    const auto idx = (size_t) a;
    return table[idx < table.size() ? idx : 1];
}
} // namespace nscm
