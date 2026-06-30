// Renders README audio examples from a CC0 source sample.

#include "PluginProcessor.h"
#include "Presets.h"

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include <cmath>
#include <memory>

struct ModeRender
{
    const char* file;
    const char* name;
    int character;
    float color;
    float amount;
    float formant;
    float gamma;
    float transient;
    float gate;
    float morph;
    float mix;
    float melody;
    float air;
};

static void setP (juce::AudioProcessorValueTreeState& s, const char* id, float value)
{
    if (auto* p = s.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (value));
}

static void applyFade (juce::AudioBuffer<float>& b, int fadeSamples)
{
    const int n = b.getNumSamples();
    fadeSamples = juce::jmin (fadeSamples, n / 2);
    for (int i = 0; i < fadeSamples; ++i)
    {
        const float in = (float) i / (float) fadeSamples;
        const float out = 1.0f - in;
        for (int ch = 0; ch < b.getNumChannels(); ++ch)
        {
            b.setSample (ch, i, b.getSample (ch, i) * in);
            b.setSample (ch, n - 1 - i, b.getSample (ch, n - 1 - i) * out);
        }
    }
}

static void normalizePeak (juce::AudioBuffer<float>& b, float peak)
{
    float maxAbs = 0.0f;
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
        for (int i = 0; i < b.getNumSamples(); ++i)
            maxAbs = juce::jmax (maxAbs, std::abs (b.getSample (ch, i)));

    if (maxAbs > 1.0e-6f)
        b.applyGain (peak / maxAbs);
}

static bool writeWav (const juce::File& file, const juce::AudioBuffer<float>& b, double sr)
{
    file.getParentDirectory().createDirectory();
    file.deleteFile();

    juce::WavAudioFormat format;
    std::unique_ptr<juce::OutputStream> stream (file.createOutputStream());
    if (stream == nullptr)
        return false;

    std::unique_ptr<juce::AudioFormatWriter> writer (
        format.createWriterFor (stream, juce::AudioFormatWriterOptions {}
                                    .withSampleRate (sr)
                                    .withNumChannels (b.getNumChannels())
                                    .withBitsPerSample (16)));
    if (writer == nullptr)
        return false;

    return writer->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
}

static juce::AudioBuffer<float> renderMode (const juce::AudioBuffer<float>& dry, double sr, const ModeRender& mode)
{
    NSColourMapAudioProcessor proc;
    proc.prepareToPlay (sr, 512);
    auto& state = proc.getState();
    nscm::presets::apply (state, nscm::presets::factory[9]); // 10 COLOR 150 Tail base
    setP (state, nscm::params::mode, 1.0f);                  // MIDI Grid
    setP (state, nscm::params::midiFreeze, 1.0f);
    setP (state, nscm::params::character, (float) mode.character);
    setP (state, nscm::params::color, mode.color);
    setP (state, nscm::params::amount, mode.amount);
    setP (state, nscm::params::formant, mode.formant);
    setP (state, nscm::params::gamma, mode.gamma);
    setP (state, nscm::params::transient, mode.transient);
    setP (state, nscm::params::gate, mode.gate);
    setP (state, nscm::params::morph, mode.morph);
    setP (state, nscm::params::mix, mode.mix);
    setP (state, nscm::params::melody, mode.melody);
    setP (state, nscm::params::air, mode.air);

    juce::AudioBuffer<float> wet;
    wet.makeCopyOf (dry);

    const int block = 512;
    int pos = 0;
    while (pos < dry.getNumSamples())
    {
        const int n = juce::jmin (block, dry.getNumSamples() - pos);
        juce::AudioBuffer<float> b (2, n);
        for (int ch = 0; ch < 2; ++ch)
            b.copyFrom (ch, 0, wet, ch, pos, n);

        juce::MidiBuffer midi;
        if (pos == 0)
        {
            midi.addEvent (juce::MidiMessage::noteOn (1, 48, (juce::uint8) 100), 0); // C3
            midi.addEvent (juce::MidiMessage::noteOn (1, 51, (juce::uint8) 100), 0); // Eb3
            midi.addEvent (juce::MidiMessage::noteOn (1, 55, (juce::uint8) 100), 0); // G3
            midi.addEvent (juce::MidiMessage::noteOn (1, 58, (juce::uint8) 100), 0); // Bb3
        }

        proc.processBlock (b, midi);

        for (int ch = 0; ch < 2; ++ch)
            wet.copyFrom (ch, pos, b, ch, 0, n);
        pos += n;
    }

    applyFade (wet, (int) std::round (0.030 * sr));
    normalizePeak (wet, 0.86f);
    return wet;
}

int main (int argc, char** argv)
{
    const juce::File source = argc > 1 ? juce::File (argv[1])
                                       : juce::File::getCurrentWorkingDirectory()
                                             .getChildFile ("samples/source/bigsoundbank_0836_urinal_flush_water_cc0.wav");
    const juce::File outDir = argc > 2 ? juce::File (argv[2])
                                       : juce::File::getCurrentWorkingDirectory().getChildFile ("samples");

    juce::AudioFormatManager formats;
    formats.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (source));
    if (reader == nullptr)
    {
        std::fprintf (stderr, "Could not read %s\n", source.getFullPathName().toRawUTF8());
        return 1;
    }

    const double sr = reader->sampleRate > 0.0 ? reader->sampleRate : 48000.0;
    const int start = juce::jmin ((int) reader->lengthInSamples, (int) std::round (0.35 * sr));
    const int length = juce::jmin ((int) reader->lengthInSamples - start, (int) std::round (8.0 * sr));
    if (length <= 0)
        return 1;

    juce::AudioBuffer<float> dry (2, length);
    dry.clear();
    reader->read (&dry, 0, length, start, true, reader->numChannels > 1);
    if (reader->numChannels == 1)
        dry.copyFrom (1, 0, dry, 0, 0, length);

    applyFade (dry, (int) std::round (0.030 * sr));
    normalizePeak (dry, 0.86f);

    static constexpr ModeRender modes[] {
        { "toilet_flush_clean_cmin7.wav", "Clean", 0, 1.05f, 0.76f,  0.0f, 0.08f, 0.62f, 0.06f, 0.26f, 0.78f, 0.22f, 0.12f },
        { "toilet_flush_color_cmin7.wav", "Color", 1, 1.24f, 0.84f,  2.0f, 0.18f, 0.55f, 0.08f, 0.32f, 0.84f, 0.42f, 0.24f },
        { "toilet_flush_hyper_cmin7.wav", "Hyper", 2, 1.45f, 0.88f,  5.0f, 0.34f, 0.48f, 0.16f, 0.22f, 0.82f, 0.55f, 0.34f },
        { "toilet_flush_map_cmin7.wav",   "Map",   3, 1.32f, 0.88f,  3.0f, 0.36f, 0.42f, 0.12f, 0.18f, 0.86f, 0.78f, 0.30f },
        { "toilet_flush_glitch_cmin7.wav","Glitch",4, 1.28f, 0.86f, 12.0f, 0.26f, 0.95f, 0.36f, 0.18f, 0.82f, 0.36f, 0.26f },
    };

    const bool okDry = writeWav (outDir.getChildFile ("toilet_flush_dry_cc0.wav"), dry, sr);
    bool okWet = true;
    for (const auto& mode : modes)
    {
        auto wet = renderMode (dry, sr, mode);
        const bool ok = writeWav (outDir.getChildFile (mode.file), wet, sr);
        okWet = okWet && ok;
        std::printf ("%s=%d file=%s\n", mode.name, (int) ok, mode.file);
        if (mode.character == 2)
            okWet = writeWav (outDir.getChildFile ("toilet_flush_nscolourmap_color150_cmin7.wav"), wet, sr) && okWet;
    }

    std::printf ("dry=%d wet=%d source=%s\n", (int) okDry, (int) okWet, source.getFullPathName().toRawUTF8());
    return (okDry && okWet) ? 0 : 1;
}
