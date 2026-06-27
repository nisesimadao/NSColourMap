#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <memory>

#include "PluginProcessor.h"

// ── LookAndFeel (preserves the NSDucking visual feel) ─────────────────────────
class NSColourMapLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    NSColourMapLookAndFeel();
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                               bool highlighted, bool down) override;
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool isDown, int bx, int by, int bw, int bh, juce::ComboBox&) override;
};

// ── DRY / TUNED / COLORED visualizer (spec §6) ────────────────────────────────
class VisualizerView final : public juce::Component, private juce::Timer
{
public:
    explicit VisualizerView (NSColourMapAudioProcessor&);
    ~VisualizerView() override;
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;
    float freqToX (float hz, juce::Rectangle<float> plot) const;
    NSColourMapAudioProcessor& processor;
    float inSm = 0.0f, tunedSm = 0.0f, colSm = 0.0f;
};

// ── Keyboard: scale / MIDI / root highlight ───────────────────────────────────
class KeyboardView final : public juce::Component, private juce::Timer
{
public:
    explicit KeyboardView (NSColourMapAudioProcessor&);
    ~KeyboardView() override;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    int pitchClassAt (juce::Point<float>) const;
    NSColourMapAudioProcessor& processor;
    juce::uint32 lastTarget = 0, lastHeld = 0;
    int lastRoot = -1;
};

// ── Editor ────────────────────────────────────────────────────────────────────
class NSColourMapAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit NSColourMapAudioProcessorEditor (NSColourMapAudioProcessor&);
    ~NSColourMapAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment   = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void configureKnob (juce::Slider&, juce::Label&, const juce::String&);
    void configureTab (juce::TextButton&, int index);
    void configureRadio (juce::TextButton&, const char* paramId, int valueIndex);
    void setCurrentTab (int);
    void timerCallback() override;
    void syncRadios();
    void updateMainVisibility();

    NSColourMapAudioProcessor& audioProcessor;
    NSColourMapLookAndFeel lnf;
    juce::TooltipWindow tooltip { this, 600 };

    // Header
    juce::Label      logoLabel;
    juce::ComboBox   presetBox;
    juce::Component  midiIndicator;
    juce::TextButton qualityButton { "0 Lat" };
    juce::TextButton advancedButton { "ADV" };
    juce::TextButton mainTab  { "Main" };
    juce::TextButton aboutTab { "About" };

    // Key strip
    std::array<juce::TextButton, 4> gridModeButtons {
        juce::TextButton { "Scale" }, juce::TextButton { "MIDI" },
        juce::TextButton { "Hybrid" }, juce::TextButton { "UI" } };
    juce::ComboBox keyBox, scaleBox;
    juce::Slider   scaleShiftKnob;
    juce::Label    scaleShiftLabel;
    juce::ToggleButton freezeButton { "Freeze" };

    // Views
    VisualizerView visualizer;
    KeyboardView   keyboard;

    // Big COLOR + main knobs
    juce::Slider colorKnob, amountKnob, formantKnob, transientKnob, mixKnob, outputKnob;
    juce::Label  colorLabel, amountLabel, formantLabel, transientLabel, mixLabel, outputLabel;

    // Character row
    std::array<juce::TextButton, 5> characterButtons {
        juce::TextButton { "Clean" }, juce::TextButton { "Color" }, juce::TextButton { "Hyper" },
        juce::TextButton { "Alien" }, juce::TextButton { "Glitch" } };

    // Advanced drawer
    juce::Slider gammaKnob, morphKnob, gateKnob, lowCutKnob, highCutKnob;
    juce::Label  gammaLabel, morphLabel, gateLabel, lowCutLabel, highCutLabel;
    juce::ToggleButton sideMuteButton { "Side Mute" }, multirateButton { "Multirate" };

    int  currentTab = 0;
    bool showAdvanced = false;

    std::unique_ptr<SliderAttachment>   colorAtt, amountAtt, formantAtt, transientAtt, mixAtt, outputAtt,
                                        scaleShiftAtt, gammaAtt, morphAtt, gateAtt, lowCutAtt, highCutAtt;
    std::unique_ptr<ComboBoxAttachment> keyAtt, scaleAtt;
    std::unique_ptr<ButtonAttachment>   freezeAtt, sideMuteAtt, multirateAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NSColourMapAudioProcessorEditor)
};
