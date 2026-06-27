#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

#include "Parameters.h"

namespace nscm::presets
{
// Factory preset = starting values (spec v0.5 §17).
struct PresetDef
{
    const char* name;
    int   mode;        // 0 Scale, 1 MIDI, 2 Hybrid, 3 UI
    int   character;   // 0 Clean .. 4 Glitch
    float color;       // 0..2 (0-200%)
    float amount;      // 0..1
    float scaleShift;  // semitones
    float formant;     // semitones
    float gamma;       // 0..1
    float transient;   // 0..1.5
    float gate;        // 0..1
    float morph;       // 0..1
    float lowCut;      // Hz
    float highCut;     // Hz
    int   key;         // 0..11
    int   scale;       // 0..11 (Scale enum)
    float mix;         // 0..1
    float output;      // dB
};

// scale: 0 Major,1 NatMinor,2 HarmMinor,3 Dorian,4 Phrygian,5 Lydian,6 Mixolydian,
//        7 MinorPent,8 MajorPent,9 PentBlues,10 WholeTone,11 Chromatic.
inline constexpr std::array<PresetDef, 20> factory {{
    //  name                    mode chr  color  amt  shift form gamma trans  gate  morph loCut  hiCut  key scl  mix   out
    { "01 Chroma Bass Start",    0,  1,  0.65f, 0.70f,  0.f,  0.f, 0.0f, 0.65f, 0.0f, 0.0f, 100.f, 6000.f, 0,  7, 0.65f, 0.f },
    { "02 Clean Scale Snap",     0,  0,  0.55f, 0.60f,  0.f,  0.f, 0.0f, 0.85f, 0.0f, 0.4f, 100.f, 7000.f, 0,  7, 0.50f, 0.f },
    { "03 MIDI Colour Chord",    1,  1,  0.80f, 0.80f,  0.f,  0.f, 0.0f, 0.60f, 0.0f, 0.2f, 110.f, 6500.f, 0,  7, 0.72f, 0.f },
    { "04 Hyper Grid Bass",      2,  2,  1.00f, 0.85f,  0.f,  0.f, 0.2f, 0.50f, 0.1f, 0.2f, 110.f, 7500.f, 0,  7, 0.75f, 0.f },
    { "05 Alien Formant Sweep",  0,  3,  0.90f, 0.80f,  0.f,  7.f, 0.5f, 0.50f, 0.1f, 0.1f, 120.f, 8000.f, 0,  9, 0.70f, 0.f },
    { "06 Glitch Laser Fill",    1,  4,  0.90f, 0.85f,  0.f, 12.f, 0.3f, 1.00f, 0.6f, 0.3f, 140.f, 9000.f, 0,  7, 0.75f, 0.f },
    { "07 Noise To Key",         0,  1,  1.00f, 1.00f,  0.f,  0.f, 0.0f, 0.40f, 0.2f, 0.0f, 100.f, 7000.f, 0,  7, 0.85f, 0.f },
    { "08 Vocal Chop Color",     1,  1,  0.80f, 0.80f,  0.f,  5.f, 0.2f, 0.55f, 0.0f, 0.5f, 120.f, 7500.f, 0,  7, 0.80f, 0.f },
    { "09 Sub Safe Growl",       1,  1,  0.70f, 0.75f,  0.f,  0.f, 0.0f, 0.60f, 0.0f, 0.3f, 160.f, 6000.f, 0,  7, 0.55f, 0.f },
    { "10 COLOR 150 Tail",       0,  2,  1.50f, 0.80f,  0.f,  0.f, 0.3f, 0.50f, 0.4f, 0.0f, 110.f, 8000.f, 0,  7, 0.70f, 0.f },
    { "11 Lydian Shimmer",       0,  2,  0.95f, 0.80f,  0.f,  0.f, 0.3f, 0.55f, 0.1f, 0.2f, 110.f, 9000.f, 0,  5, 0.70f, 0.f },
    { "12 Dark Phrygian Growl",  0,  1,  0.85f, 0.80f,  0.f, -3.f, 0.2f, 0.55f, 0.1f, 0.3f, 130.f, 6500.f, 0,  4, 0.65f, 0.f },
    { "13 Talking Formant",      1,  3,  0.85f, 0.80f,  0.f,  0.f, 0.8f, 0.55f, 0.1f, 0.2f, 120.f, 8000.f, 0,  7, 0.78f, 0.f },
    { "14 Crystal Pluck",        1,  2,  1.10f, 0.85f,  0.f,  9.f, 0.4f, 0.80f, 0.4f, 0.5f, 130.f,10000.f, 0,  8, 0.72f, 0.f },
    { "15 Hybrid Key Wash",      2,  1,  0.75f, 0.70f,  0.f,  0.f, 0.1f, 0.55f, 0.0f, 0.3f, 110.f, 6500.f, 0,  1, 0.60f, 0.f },
    { "16 Whole Tone Dream",     0,  3,  0.90f, 0.80f,  0.f,  4.f, 0.6f, 0.45f, 0.1f, 0.2f, 120.f, 8000.f, 0, 10, 0.68f, 0.f },
    { "17 Tight Transient Bass", 1,  0,  0.70f, 0.70f,  0.f,  0.f, 0.0f, 1.10f, 0.2f, 0.7f, 110.f, 7000.f, 0,  7, 0.55f, 0.f },
    { "18 MIDI Chord Stab",      1,  2,  1.05f, 0.85f,  0.f,  0.f, 0.2f, 0.85f, 0.5f, 0.5f, 110.f, 8000.f, 0,  7, 0.78f, 0.f },
    { "19 Broken Alien Tail",    1,  3,  1.40f, 0.85f,  0.f,  0.f, 0.7f, 0.40f, 0.2f, 0.0f, 130.f, 9000.f, 0,  9, 0.80f, 0.f },
    { "20 Soft Major Glow",      0,  0,  0.60f, 0.60f,  0.f,  0.f, 0.1f, 0.70f, 0.0f, 0.4f, 100.f, 7000.f, 0,  0, 0.45f, 0.f },
}};

inline void setParam (juce::AudioProcessorValueTreeState& s, const char* id, float value)
{
    if (auto* p = s.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (value));
        p->endChangeGesture();
    }
}

inline void apply (juce::AudioProcessorValueTreeState& s, const PresetDef& d)
{
    setParam (s, params::mode,       (float) d.mode);
    setParam (s, params::character,  (float) d.character);
    setParam (s, params::color,      d.color);
    setParam (s, params::amount,     d.amount);
    setParam (s, params::scaleShift, d.scaleShift);
    setParam (s, params::formant,    d.formant);
    setParam (s, params::gamma,      d.gamma);
    setParam (s, params::transient,  d.transient);
    setParam (s, params::gate,       d.gate);
    setParam (s, params::morph,      d.morph);
    setParam (s, params::lowCut,     d.lowCut);
    setParam (s, params::highCut,    d.highCut);
    setParam (s, params::key,        (float) d.key);
    setParam (s, params::scale,      (float) d.scale);
    setParam (s, params::mix,        d.mix);
    setParam (s, params::output,     d.output);
}
} // namespace nscm::presets
