#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

#include "Parameters.h"

namespace nscm::presets
{
// A factory preset = a full set of starting values (spec §23 names, §27 values).
struct PresetDef
{
    const char* name;
    int   mode;        // 0 = MIDI Chord, 1 = Scale Resonance
    int   algo;        // 0 Clean .. 4 Broken
    float amount;      // 0..1
    float glide;       // ms
    float colour;      // 0..1
    float formant;     // semitones
    float subProtect;  // Hz
    float transient;   // 0..1.5
    float mix;         // 0..1
    float output;      // dB
    bool  freeze;
    int   key;         // 0..11
    int   scale;       // 0..9 (Scale enum order)
    int   scaleShift;  // semitones
};

// Mode 0=MIDI Chord, 1=Scale Resonance. Algo 0 Clean,1 Colour,2 Hyper,3 HiTECH,4 Broken.
// Scale 7 = Minor Pentatonic, 9 = Pentatonic Blues, 1 = Natural Minor.
inline constexpr std::array<PresetDef, 10> factory {{
    //  name                  mode algo  amt  glide  col  form   sub  trans  mix   out  frz key scl shift
    { "01 MIDI Colour Bass",   0,  1,  0.70f,  45.f, 0.60f,  0.f, 120.f, 0.60f, 0.65f, 0.f, true, 0, 7,  0 },
    { "02 Soft Chord Wash",    1,  0,  0.45f,  90.f, 0.30f,  0.f, 120.f, 0.55f, 0.40f, 0.f, true, 0, 7,  0 },
    { "03 Resonant Growl",     0,  1,  0.82f,  30.f, 0.72f, -3.f, 120.f, 0.50f, 0.70f, 0.f, true, 0, 7,  0 },
    { "04 Hyper Rainbow",      0,  2,  0.85f,  20.f, 0.85f,  7.f, 130.f, 0.45f, 0.75f, 0.f, true, 0, 7,  0 },
    { "05 HiTECH Laser Fill",  0,  3,  0.80f,  12.f, 0.85f, 12.f, 140.f, 0.85f, 0.75f, 0.f, true, 0, 7,  0 },
    { "06 Broken Formant",     0,  4,  0.90f,   8.f, 0.90f,  0.f, 150.f, 0.20f, 0.90f, 0.f, true, 0, 7,  0 },
    { "07 Noise To Chord",     0,  2,  0.88f,  25.f, 0.82f,  5.f, 120.f, 0.20f, 1.00f, 0.f, true, 0, 7,  0 },
    { "08 Sub Safe Bass",      0,  1,  0.60f,  60.f, 0.45f,  0.f, 150.f, 0.60f, 0.50f, 0.f, true, 0, 7,  0 },
    { "09 Pentatonic Shine",   1,  2,  0.80f,  35.f, 0.70f,  0.f, 130.f, 0.50f, 0.55f, 0.f, true, 0, 7,  0 },
    { "10 Vocal Robot Chord",  0,  1,  0.75f,  18.f, 0.65f,  9.f, 120.f, 0.45f, 0.80f, 0.f, true, 0, 7,  0 },
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
    setParam (s, params::mode,        (float) d.mode);
    setParam (s, params::algo,        (float) d.algo);
    setParam (s, params::amount,      d.amount);
    setParam (s, params::glide,       d.glide);
    setParam (s, params::colour,      d.colour);
    setParam (s, params::formant,     d.formant);
    setParam (s, params::subProtect,  d.subProtect);
    setParam (s, params::transient,   d.transient);
    setParam (s, params::mix,         d.mix);
    setParam (s, params::output,      d.output);
    setParam (s, params::freezeChord, d.freeze ? 1.0f : 0.0f);
    setParam (s, params::key,         (float) d.key);
    setParam (s, params::scale,       (float) d.scale);
    setParam (s, params::scaleShift,  (float) d.scaleShift);
    // Quality and uiTab are intentionally left as the user set them.
}
} // namespace nscm::presets
