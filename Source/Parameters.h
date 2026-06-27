#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace nscm::params
{
// Parameter IDs (spec §22).
inline constexpr auto mode        = "mode";
inline constexpr auto algo        = "algo";
inline constexpr auto amount      = "amount";
inline constexpr auto glide       = "glide";
inline constexpr auto colour      = "colour";
inline constexpr auto formant     = "formant";
inline constexpr auto subProtect  = "subProtect";
inline constexpr auto transient   = "transient";
inline constexpr auto mix         = "mix";
inline constexpr auto output      = "output";
inline constexpr auto freezeChord = "freezeChord";
inline constexpr auto key         = "key";
inline constexpr auto scale       = "scale";
inline constexpr auto scaleShift  = "scaleShift";
inline constexpr auto quality     = "quality";
inline constexpr auto uiTab       = "uiTab";

inline juce::StringArray modeNames()  { return { "MIDI Chord", "Scale Resonance" }; }
inline juce::StringArray algoNames()  { return { "Clean", "Colour", "Hyper", "HiTECH", "Broken" }; }
inline juce::StringArray keyNames()   { return { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }; }
inline juce::StringArray scaleNames() { return { "Major", "Natural Minor", "Harmonic Minor", "Dorian", "Phrygian",
                                                 "Lydian", "Mixolydian", "Minor Pentatonic", "Major Pentatonic",
                                                 "Pentatonic Blues" }; }
inline juce::StringArray qualityNames() { return { "Eco", "Normal", "High" }; }

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
} // namespace nscm::params
