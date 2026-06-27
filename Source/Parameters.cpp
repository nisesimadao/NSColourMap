#include "Parameters.h"

namespace nscm::params
{
using APF  = juce::AudioParameterFloat;
using APC  = juce::AudioParameterChoice;
using APB  = juce::AudioParameterBool;
using Attr = juce::AudioParameterFloatAttributes;

static juce::String percentString (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)); }

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    p.push_back (std::make_unique<APC> (juce::ParameterID { mode, 1 }, "Mode", modeNames(), 0));
    p.push_back (std::make_unique<APC> (juce::ParameterID { algo, 1 }, "Algo", algoNames(), 1));

    p.push_back (std::make_unique<APF> (juce::ParameterID { amount, 1 }, "Amount",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.70f,
        Attr().withLabel ("%").withStringFromValueFunction (percentString)));

    p.push_back (std::make_unique<APF> (juce::ParameterID { glide, 1 }, "Glide",
        juce::NormalisableRange<float> { 0.0f, 500.0f, 0.1f, 0.5f }, 45.0f,
        Attr().withLabel ("ms")));

    p.push_back (std::make_unique<APF> (juce::ParameterID { colour, 1 }, "Colour",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.55f,
        Attr().withLabel ("%").withStringFromValueFunction (percentString)));

    p.push_back (std::make_unique<APF> (juce::ParameterID { formant, 1 }, "Formant",
        juce::NormalisableRange<float> { -24.0f, 24.0f, 0.1f }, 0.0f,
        Attr().withLabel ("st")));

    p.push_back (std::make_unique<APF> (juce::ParameterID { subProtect, 1 }, "Sub Protect",
        juce::NormalisableRange<float> { 40.0f, 200.0f, 1.0f }, 120.0f,
        Attr().withLabel ("Hz")));

    p.push_back (std::make_unique<APF> (juce::ParameterID { transient, 1 }, "Transient",
        juce::NormalisableRange<float> { 0.0f, 1.5f, 0.001f }, 0.60f,
        Attr().withLabel ("%").withStringFromValueFunction (percentString)));

    p.push_back (std::make_unique<APF> (juce::ParameterID { mix, 1 }, "Mix",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.65f,
        Attr().withLabel ("%").withStringFromValueFunction (percentString)));

    p.push_back (std::make_unique<APF> (juce::ParameterID { output, 1 }, "Output",
        juce::NormalisableRange<float> { -12.0f, 12.0f, 0.01f }, 0.0f,
        Attr().withLabel ("dB")));

    p.push_back (std::make_unique<APB> (juce::ParameterID { freezeChord, 1 }, "Freeze", true));
    p.push_back (std::make_unique<APC> (juce::ParameterID { key, 1 }, "Key", keyNames(), 0));
    p.push_back (std::make_unique<APC> (juce::ParameterID { scale, 1 }, "Scale", scaleNames(), 7)); // Minor Pentatonic

    p.push_back (std::make_unique<APF> (juce::ParameterID { scaleShift, 1 }, "Scale Shift",
        juce::NormalisableRange<float> { -12.0f, 12.0f, 1.0f }, 0.0f,
        Attr().withLabel ("st")));

    p.push_back (std::make_unique<APC> (juce::ParameterID { quality, 1 }, "Quality", qualityNames(), 1));
    p.push_back (std::make_unique<APC> (juce::ParameterID { uiTab, 1 }, "UI Tab", juce::StringArray { "Main", "About" }, 0));

    return { p.begin(), p.end() };
}
} // namespace nscm::params
