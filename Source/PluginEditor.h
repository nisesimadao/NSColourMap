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

// Buttons that hold eased animation state for select/hover transitions.
struct AnimatedButton final : public juce::TextButton
{
    using juce::TextButton::TextButton;
    float sel = 0.0f, hover = 0.0f;
};
struct AnimatedToggle final : public juce::ToggleButton
{
    using juce::ToggleButton::ToggleButton;
    float sel = 0.0f, hover = 0.0f;
};

// Advances a button's sel/hover toward its target with easing; returns true if it
// changed enough to need a repaint.
template <typename B>
inline bool stepButtonAnim (B& b)
{
    const float ts = b.getToggleState() ? 1.0f : 0.0f;
    const float th = b.isOver() ? 1.0f : 0.0f;
    const float ns = b.sel   + (ts > b.sel   ? 0.40f : 0.18f) * (ts - b.sel);
    const float nh = b.hover + (th > b.hover ? 0.45f : 0.22f) * (th - b.hover);
    bool changed = false;
    if (std::abs (ns - b.sel)   > 0.004f) { b.sel = ns;     changed = true; } else b.sel = ts;
    if (std::abs (nh - b.hover) > 0.004f) { b.hover = nh;   changed = true; } else b.hover = th;
    return changed;
}

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
    void layoutClassic (juce::Rectangle<int> area);
    void layoutClean (juce::Rectangle<int> area);
    void setUiStyle (int style);

    NSColourMapAudioProcessor& audioProcessor;
    NSColourMapLookAndFeel lnf;
    juce::TooltipWindow tooltip { this, 600 };

    // Header
    juce::Label      logoLabel;
    juce::ComboBox   presetBox;
    juce::Component  midiIndicator;
    juce::Slider     qualitySlider;   // 0 Latency .. High (latency/quality)
    juce::Label      qualityLabel;
    AnimatedButton   advancedButton { "ADV" };
    AnimatedButton   styleButton { "Clean" };
    AnimatedButton   mainTab  { "Main" };
    AnimatedButton   aboutTab { "About" };

    // Key strip
    std::array<AnimatedButton, 5> gridModeButtons {
        AnimatedButton { "Scale" }, AnimatedButton { "MIDI" },
        AnimatedButton { "Hybrid" }, AnimatedButton { "UI" }, AnimatedButton { "Audio" } };
    juce::ComboBox keyBox, scaleBox;
    juce::Slider   scaleShiftKnob;
    juce::Label    scaleShiftLabel;
    AnimatedToggle freezeButton { "Freeze" };

    // Views
    VisualizerView visualizer;
    KeyboardView   keyboard;

    // Big COLOR + main knobs
    juce::Slider colorKnob, amountKnob, formantKnob, transientKnob, mixKnob, outputKnob;
    juce::Label  colorLabel, amountLabel, formantLabel, transientLabel, mixLabel, outputLabel;

    // Character row
    std::array<AnimatedButton, 5> characterButtons {
        AnimatedButton { "Clean" }, AnimatedButton { "Color" }, AnimatedButton { "Hyper" },
        AnimatedButton { "Alien" }, AnimatedButton { "Glitch" } };

    // Advanced drawer
    juce::Slider gammaKnob, morphKnob, gateKnob, lowCutKnob, highCutKnob;
    juce::Label  gammaLabel, morphLabel, gateLabel, lowCutLabel, highCutLabel;
    AnimatedToggle sideMuteButton { "Side Mute" }, multirateButton { "Multirate" };

    int   currentTab = 0;
    bool  showAdvanced = false;
    int   uiStyle = 0; // 0 = Clean (default), 1 = Classic
    float glowLevel = 0.0f; // eased COLOR glow, driven by live colour energy

    // Clean-layout section rectangles (drawn as labelled panels in paint()).
    juce::Rectangle<int> rSourcePanel, rEnginePanel,
                         rTitleSource, rTitleChar, rTitleColor, rTitleTone;

    std::unique_ptr<SliderAttachment>   colorAtt, amountAtt, formantAtt, transientAtt, mixAtt, outputAtt,
                                        scaleShiftAtt, gammaAtt, morphAtt, gateAtt, lowCutAtt, highCutAtt, qualityAtt;
    std::unique_ptr<ComboBoxAttachment> keyAtt, scaleAtt;
    std::unique_ptr<ButtonAttachment>   freezeAtt, sideMuteAtt, multirateAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NSColourMapAudioProcessorEditor)
};
