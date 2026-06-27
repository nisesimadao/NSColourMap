#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <memory>
#include <vector>

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

// ── Spectrum / pitch-lane diagnostic view (spec §12.4) ────────────────────────
class SpectrumMapView final : public juce::Component, private juce::Timer
{
public:
    explicit SpectrumMapView (NSColourMapAudioProcessor&);
    ~SpectrumMapView() override;
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;
    NSColourMapAudioProcessor& processor;
    float energySmoothed = 0.0f;
};

// ── Keyboard view: highlights target + held pitch classes ─────────────────────
class KeyboardView final : public juce::Component, private juce::Timer
{
public:
    explicit KeyboardView (NSColourMapAudioProcessor&);
    ~KeyboardView() override;
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;
    NSColourMapAudioProcessor& processor;
    juce::uint32 lastTarget = 0, lastHeld = 0;
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
    void configureRadio (juce::TextButton&, const char* paramId, int valueIndex, int radioGroup);
    void setCurrentTab (int);
    void timerCallback() override;
    void syncRadios();
    void handleSnapshot (int index);

    NSColourMapAudioProcessor& audioProcessor;
    NSColourMapLookAndFeel lnf;
    juce::TooltipWindow tooltip { this, 600 };

    // Preset selector
    juce::ComboBox presetBox;

    // Header
    juce::Label      logoLabel;
    juce::TextButton mainTab  { "Main" };
    juce::TextButton aboutTab { "About" };
    juce::Component  midiIndicator;
    juce::Label      statusLabel;

    // Views
    SpectrumMapView spectrum;
    KeyboardView    keyboard;

    // Mode + Algo radios
    juce::TextButton chordModeBtn { "MIDI Chord" };
    juce::TextButton scaleModeBtn { "Scale Resonance" };
    std::array<juce::TextButton, 5> algoButtons {
        juce::TextButton { "Clean" }, juce::TextButton { "Colour" }, juce::TextButton { "Hyper" },
        juce::TextButton { "HiTECH" }, juce::TextButton { "Broken" } };

    // Key / Scale / Quality / Freeze / Scale Shift
    juce::ComboBox keyBox, scaleBox, qualityBox;
    juce::Label    keyLabel, scaleLabel, qualityLabel, scaleShiftLabel;
    juce::ToggleButton freezeButton { "Freeze" };
    juce::Slider   scaleShiftSlider;

    // Macro knobs
    juce::Slider amountKnob, glideKnob, colourKnob, formantKnob;
    juce::Label  amountLabel, glideLabel, colourLabel, formantLabel;
    // Utility knobs
    juce::Slider subKnob, transientKnob, mixKnob, outputKnob;
    juce::Label  subLabel, transientLabel, mixLabel, outputLabel;

    // Snapshot placeholders (spec v1)
    std::array<juce::TextButton, 4> snapshots {
        juce::TextButton { "A" }, juce::TextButton { "B" }, juce::TextButton { "C" }, juce::TextButton { "D" } };

    int currentTab = 0;

    std::unique_ptr<SliderAttachment>   amountAtt, glideAtt, colourAtt, formantAtt,
                                        subAtt, transientAtt, mixAtt, outputAtt, scaleShiftAtt;
    std::unique_ptr<ComboBoxAttachment> keyAtt, scaleAtt, qualityAtt;
    std::unique_ptr<ButtonAttachment>   freezeAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NSColourMapAudioProcessorEditor)
};
