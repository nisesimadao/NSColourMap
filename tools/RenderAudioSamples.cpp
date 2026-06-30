// Renders README audio examples from a CC0 source sample.

#include "PluginProcessor.h"
#include "Presets.h"

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include <cmath>
#include <memory>

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
    std::unique_ptr<juce::FileOutputStream> stream (file.createOutputStream());
    if (stream == nullptr)
        return false;

    std::unique_ptr<juce::AudioFormatWriter> writer (
        format.createWriterFor (stream.get(), sr, (unsigned int) b.getNumChannels(), 16, {}, 0));
    if (writer == nullptr)
        return false;

    stream.release();
    return writer->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
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

    NSColourMapAudioProcessor proc;
    proc.prepareToPlay (sr, 512);
    auto& state = proc.getState();
    nscm::presets::apply (state, nscm::presets::factory[9]); // 10 COLOR 150 Tail
    setP (state, nscm::params::mode, 1.0f);                  // MIDI Grid
    setP (state, nscm::params::midiFreeze, 1.0f);
    setP (state, nscm::params::mix, 0.86f);
    setP (state, nscm::params::melody, 0.65f);
    setP (state, nscm::params::air, 0.28f);

    juce::AudioBuffer<float> wet;
    wet.makeCopyOf (dry);

    const int block = 512;
    int pos = 0;
    while (pos < length)
    {
        const int n = juce::jmin (block, length - pos);
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

    const bool okDry = writeWav (outDir.getChildFile ("toilet_flush_dry_cc0.wav"), dry, sr);
    const bool okWet = writeWav (outDir.getChildFile ("toilet_flush_nscolourmap_color150_cmin7.wav"), wet, sr);
    std::printf ("dry=%d wet=%d source=%s\n", (int) okDry, (int) okWet, source.getFullPathName().toRawUTF8());
    return (okDry && okWet) ? 0 : 1;
}
