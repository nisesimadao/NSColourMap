#include "Parameters.h"

namespace nscm::params
{
using APF  = juce::AudioParameterFloat;
using APC  = juce::AudioParameterChoice;
using APB  = juce::AudioParameterBool;
using Attr = juce::AudioParameterFloatAttributes;

static juce::String percentString (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)); }
static juce::String hzString (float v, int)      { return juce::String (juce::roundToInt (v)) + " Hz"; }

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    p.push_back (std::make_unique<APC> (juce::ParameterID { mode, 1 }, "Grid Mode", modeNames(), 0));        // Scale
    p.push_back (std::make_unique<APC> (juce::ParameterID { character, 1 }, "Character", characterNames(), 1)); // Color

    // Big COLOR — 0..200%. Default high enough that the effect is obvious out of the box.
    p.push_back (std::make_unique<APF> (juce::ParameterID { color, 1 }, "COLOR",
        juce::NormalisableRange<float> { 0.0f, 2.0f, 0.001f }, 0.90f,
        Attr().withLabel ("%").withStringFromValueFunction (percentString)));

    p.push_back (std::make_unique<APF> (juce::ParameterID { amount, 1 }, "Amount",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.85f,
        Attr().withLabel ("%").withStringFromValueFunction (percentString)));

    p.push_back (std::make_unique<APF> (juce::ParameterID { melody, 1 }, "Melody",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.0f,
        Attr().withLabel ("%").withStringFromValueFunction (percentString)));

    p.push_back (std::make_unique<APF> (juce::ParameterID { scaleShift, 1 }, "Scale Shift",
        juce::NormalisableRange<float> { -12.0f, 12.0f, 1.0f }, 0.0f, Attr().withLabel ("st")));

    p.push_back (std::make_unique<APF> (juce::ParameterID { formant, 1 }, "Formant",
        juce::NormalisableRange<float> { -24.0f, 24.0f, 0.1f }, 0.0f, Attr().withLabel ("st")));

    p.push_back (std::make_unique<APF> (juce::ParameterID { gamma, 1 }, "Gamma",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.0f,
        Attr().withLabel ("%").withStringFromValueFunction (percentString)));

    p.push_back (std::make_unique<APF> (juce::ParameterID { transient, 1 }, "Transient",
        juce::NormalisableRange<float> { 0.0f, 1.5f, 0.001f }, 0.65f,
        Attr().withLabel ("%").withStringFromValueFunction (percentString)));

    p.push_back (std::make_unique<APF> (juce::ParameterID { morph, 1 }, "Morph",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.0f,
        Attr().withLabel ("%").withStringFromValueFunction (percentString)));

    p.push_back (std::make_unique<APF> (juce::ParameterID { gate, 1 }, "Gate",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.0f,
        Attr().withLabel ("%").withStringFromValueFunction (percentString)));

    p.push_back (std::make_unique<APF> (juce::ParameterID { air, 1 }, "Air",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.15f,
        Attr().withLabel ("%").withStringFromValueFunction (percentString)));

    p.push_back (std::make_unique<APF> (juce::ParameterID { lowCut, 1 }, "Low Cut",
        juce::NormalisableRange<float> { 40.0f, 500.0f, 1.0f, 0.5f }, 100.0f,
        Attr().withStringFromValueFunction (hzString)));

    p.push_back (std::make_unique<APF> (juce::ParameterID { highCut, 1 }, "High Cut",
        juce::NormalisableRange<float> { 1000.0f, 16000.0f, 1.0f, 0.4f }, 6000.0f,
        Attr().withStringFromValueFunction (hzString)));

    p.push_back (std::make_unique<APB> (juce::ParameterID { sideMute, 1 }, "Side Mute", false));
    p.push_back (std::make_unique<APC> (juce::ParameterID { key, 1 }, "Key", keyNames(), 0));
    p.push_back (std::make_unique<APC> (juce::ParameterID { scale, 1 }, "Scale", scaleNames(), 7)); // Minor Pentatonic
    p.push_back (std::make_unique<APB> (juce::ParameterID { midiFreeze, 1 }, "Freeze", false));
    p.push_back (std::make_unique<APC> (juce::ParameterID { quality, 1 }, "Quality", qualityNames(), 0)); // 0 Latency

    p.push_back (std::make_unique<APF> (juce::ParameterID { mix, 1 }, "Mix",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.78f,
        Attr().withLabel ("%").withStringFromValueFunction (percentString)));

    p.push_back (std::make_unique<APF> (juce::ParameterID { output, 1 }, "Output",
        juce::NormalisableRange<float> { -12.0f, 12.0f, 0.01f }, 0.0f, Attr().withLabel ("dB")));

    p.push_back (std::make_unique<APB> (juce::ParameterID { multirate, 1 }, "Multirate", false));
    p.push_back (std::make_unique<APC> (juce::ParameterID { uiTab, 1 }, "UI Tab", juce::StringArray { "Main", "About" }, 0));

    return { p.begin(), p.end() };
}
} // namespace nscm::params
