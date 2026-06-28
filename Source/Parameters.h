#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace nscm::params
{
// Parameter IDs (spec v0.5 §13).
inline constexpr auto mode        = "mode";        // Grid Mode
inline constexpr auto character   = "character";   // Character mode
inline constexpr auto color       = "color";       // Big COLOR 0..200%
inline constexpr auto amount      = "amount";
inline constexpr auto melody      = "melody";
inline constexpr auto scaleShift  = "scaleShift";
inline constexpr auto formant     = "formant";
inline constexpr auto gamma       = "gamma";
inline constexpr auto transient   = "transient";
inline constexpr auto morph       = "morph";
inline constexpr auto gate        = "gate";
inline constexpr auto air         = "air";
inline constexpr auto lowCut      = "lowCut";
inline constexpr auto highCut     = "highCut";
inline constexpr auto sideMute    = "sideMute";
inline constexpr auto key         = "key";
inline constexpr auto scale       = "scale";
inline constexpr auto midiFreeze  = "midiFreeze";
inline constexpr auto quality     = "quality";     // 0 Latency / High Quality
inline constexpr auto mix         = "mix";
inline constexpr auto output      = "output";
inline constexpr auto multirate   = "multirate";
inline constexpr auto uiTab       = "uiTab";

inline juce::StringArray modeNames()      { return { "Scale", "MIDI", "Hybrid", "UI", "Audio" }; }
inline juce::StringArray characterNames() { return { "Clean", "Color", "Hyper", "Alien", "Glitch" }; }
inline juce::StringArray keyNames()       { return { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }; }
inline juce::StringArray scaleNames()     { return { "Major", "Natural Minor", "Harmonic Minor", "Dorian", "Phrygian",
                                                     "Lydian", "Mixolydian", "Minor Pentatonic", "Major Pentatonic",
                                                     "Pentatonic Blues", "Whole Tone", "Chromatic" }; }
// Quality / latency slider: 0 Latency (oscillator core) -> Low/Mid/High STFT tiers.
inline juce::StringArray qualityNames()   { return { "0 Latency", "Low", "Mid", "High" }; }

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
} // namespace nscm::params
