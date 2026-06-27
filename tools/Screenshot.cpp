// Renders the plugin editor to PNG files for the README (no DAW needed).
// Builds as a small JUCE program; uses createComponentSnapshot so no window is shown.

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <juce_gui_basics/juce_gui_basics.h>
#include <random>

static void setP (juce::AudioProcessorValueTreeState& s, const char* id, float value)
{
    if (auto* p = s.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (value));
}

static void savePng (juce::Component& c, const juce::File& file)
{
    const auto img = c.createComponentSnapshot (c.getLocalBounds(), true, 2.0f); // 2x for crisp
    file.getParentDirectory().createDirectory();
    file.deleteFile();
    if (auto os = file.createOutputStream())
    {
        juce::PNGImageFormat png;
        png.writeImageToStream (img, *os);
    }
    std::printf ("wrote %s (%d x %d)\n", file.getFullPathName().toRawUTF8(),
                 img.getWidth(), img.getHeight());
}

static void prime (NSColourMapAudioProcessor& proc)
{
    // Feed a little noise so the visualizer/energy meters show activity.
    std::mt19937 rng (7);
    std::uniform_real_distribution<float> d (-0.25f, 0.25f);
    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;
    for (int b = 0; b < 60; ++b)
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            auto* w = buf.getWritePointer (ch);
            for (int i = 0; i < 512; ++i) w[i] = d (rng);
        }
        proc.processBlock (buf, midi);
    }
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI guiInit;

    const juce::File outDir = argc > 1 ? juce::File (argv[1])
                                       : juce::File::getCurrentWorkingDirectory().getChildFile ("images");

    NSColourMapAudioProcessor proc;
    proc.prepareToPlay (48000.0, 512);

    auto& s = proc.getState();
    setP (s, nscm::params::mode,      0.0f); // Scale
    setP (s, nscm::params::character, 2.0f); // Hyper (shiny)
    setP (s, nscm::params::color,     1.10f);
    setP (s, nscm::params::amount,    0.85f);
    setP (s, nscm::params::key,       0.0f); // C
    setP (s, nscm::params::scale,     7.0f); // Minor Pentatonic
    prime (proc);

    // Main page
    {
        std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
        ed->setSize (820, 680);
        juce::MessageManager::getInstance()->runDispatchLoopUntil (350);
        savePng (*ed, outDir.getChildFile ("screenshot_main.png"));
    }

    // About page
    setP (s, nscm::params::uiTab, 1.0f);
    {
        std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
        ed->setSize (820, 680);
        juce::MessageManager::getInstance()->runDispatchLoopUntil (250);
        savePng (*ed, outDir.getChildFile ("screenshot_about.png"));
    }

    return 0;
}
