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
    float lowCut;      // Hz
    float highCut;     // Hz
    int   key;         // 0..11
    int   scale;       // 0..11 (Scale enum)
    float mix;         // 0..1
    float output;      // dB
};

// scale 7 = Minor Pentatonic, 1 = Natural Minor, 11 = Chromatic.
inline constexpr std::array<PresetDef, 10> factory {{
    //  name                  mode chr  color  amt  shift form gamma trans  gate  loCut  hiCut  key scl  mix   out
    { "01 Chroma Bass Start",  0,  1,  0.65f, 0.70f,  0.f,  0.f, 0.0f, 0.65f, 0.0f, 100.f, 6000.f, 0,  7, 0.65f, 0.f },
    { "02 Clean Scale Snap",   0,  0,  0.55f, 0.60f,  0.f,  0.f, 0.0f, 0.85f, 0.0f, 100.f, 7000.f, 0,  7, 0.50f, 0.f },
    { "03 MIDI Colour Chord",  1,  1,  0.80f, 0.80f,  0.f,  0.f, 0.0f, 0.60f, 0.0f, 110.f, 6500.f, 0,  7, 0.72f, 0.f },
    { "04 Hyper Grid Bass",    2,  2,  1.00f, 0.85f,  0.f,  0.f, 0.2f, 0.50f, 0.1f, 110.f, 7500.f, 0,  7, 0.75f, 0.f },
    { "05 Alien Formant Sweep",0,  3,  0.90f, 0.80f,  0.f,  7.f, 0.5f, 0.50f, 0.1f, 120.f, 8000.f, 0,  9, 0.70f, 0.f },
    { "06 Glitch Laser Fill",  1,  4,  0.90f, 0.85f,  0.f, 12.f, 0.3f, 1.00f, 0.6f, 140.f, 9000.f, 0,  7, 0.75f, 0.f },
    { "07 Noise To Key",       0,  1,  1.00f, 1.00f,  0.f,  0.f, 0.0f, 0.40f, 0.2f, 100.f, 7000.f, 0,  7, 0.85f, 0.f },
    { "08 Vocal Chop Color",   1,  1,  0.80f, 0.80f,  0.f,  5.f, 0.2f, 0.55f, 0.0f, 120.f, 7500.f, 0,  7, 0.80f, 0.f },
    { "09 Sub Safe Growl",     1,  1,  0.70f, 0.75f,  0.f,  0.f, 0.0f, 0.60f, 0.0f, 160.f, 6000.f, 0,  7, 0.55f, 0.f },
    { "10 COLOR 150 Tail",     0,  2,  1.50f, 0.80f,  0.f,  0.f, 0.3f, 0.50f, 0.4f, 110.f, 8000.f, 0,  7, 0.70f, 0.f },
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
    setParam (s, params::lowCut,     d.lowCut);
    setParam (s, params::highCut,    d.highCut);
    setParam (s, params::key,        (float) d.key);
    setParam (s, params::scale,      (float) d.scale);
    setParam (s, params::mix,        d.mix);
    setParam (s, params::output,     d.output);
}
} // namespace nscm::presets
