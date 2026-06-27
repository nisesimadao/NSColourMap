#include "PluginEditor.h"

#include "Parameters.h"
#include "Presets.h"
#include "dsp/AlgoModes.h"
#include "dsp/ScaleNoteSet.h"

#include <cmath>

namespace
{
const juce::Colour background { 0xff15171a };
const juce::Colour panel      { 0xff24282d };
const juce::Colour panelLight { 0xff30363d };
const juce::Colour text       { 0xfff2f5f8 };
const juce::Colour mutedText  { 0xff9aa4ad };
const juce::Colour accent     { 0xff38c7f3 };
const juce::Colour hotAccent  { 0xffff715b };

#if JUCE_WINDOWS
constexpr const char* kSansRegular = "Segoe UI";
constexpr const char* kSansLight   = "Segoe UI Light";
#else
constexpr const char* kSansRegular = "Helvetica Neue";
constexpr const char* kSansLight   = "Helvetica Neue";
#endif

juce::Font labelFont()   { return juce::Font (juce::FontOptions{}.withName (kSansLight).withHeight (12.0f).withStyle ("Light")); }
juce::Font titleFont()   { return juce::Font (juce::FontOptions{}.withName (kSansLight).withHeight (22.0f).withStyle ("Light")); }
juce::Font sectionFont() { return juce::Font (juce::FontOptions{}.withName (kSansRegular).withHeight (11.0f)); }

int choiceIndex (juce::AudioProcessorValueTreeState& p, const char* id)
{
    if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (p.getParameter (id)))
        return c->getIndex();
    return 0;
}

void setChoice (juce::AudioProcessorValueTreeState& p, const char* id, int index)
{
    if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (p.getParameter (id)))
    {
        c->beginChangeGesture();
        c->setValueNotifyingHost (c->convertTo0to1 ((float) index));
        c->endChangeGesture();
    }
}

const char* kNoteNames[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
} // namespace

// ── LookAndFeel ───────────────────────────────────────────────────────────────

NSColourMapLookAndFeel::NSColourMapLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId,         text);
    setColour (juce::TextButton::textColourOffId,         mutedText);
    setColour (juce::TextButton::textColourOnId,          text);
    setColour (juce::ComboBox::backgroundColourId,        panelLight);
    setColour (juce::ComboBox::textColourId,              text);
    setColour (juce::ComboBox::outlineColourId,           juce::Colour { 0xff3d4450u });
    setColour (juce::PopupMenu::backgroundColourId,       panel);
    setColour (juce::PopupMenu::textColourId,             text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, accent.withAlpha (0.3f));
    setColour (juce::ToggleButton::textColourId,          text);
    setColour (juce::ToggleButton::tickColourId,          accent);
    setColour (juce::ToggleButton::tickDisabledColourId,  mutedText);
    setColour (juce::Label::textColourId,                 text);
}

void NSColourMapLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                                   const juce::Colour&, bool highlighted, bool /*down*/)
{
    const auto bounds   = button.getLocalBounds().toFloat();
    const auto selected = button.getToggleState();

    if (selected)
    {
        g.setColour (accent.withAlpha (0.12f));
        g.fillRoundedRectangle (bounds.reduced (1.0f, 2.0f), 4.0f);
    }
    else if (highlighted)
    {
        g.setColour (juce::Colours::white.withAlpha (0.04f));
        g.fillRoundedRectangle (bounds.reduced (1.0f, 2.0f), 4.0f);
    }

    if (selected)
    {
        g.setColour (accent);
        g.fillRoundedRectangle (
            juce::Rectangle<float> (bounds.getX() + 8.0f, bounds.getBottom() - 3.0f,
                                    bounds.getWidth() - 16.0f, 2.5f), 1.2f);
    }
}

void NSColourMapLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                          int, int, int, int, juce::ComboBox&)
{
    const auto bounds = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);
    g.setColour (panelLight);
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (juce::Colour { 0xff3d4450u });
    g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);

    const float cx = (float) width - 14.0f;
    const float cy = (float) height * 0.5f;
    juce::Path chevron;
    chevron.startNewSubPath (cx - 4.0f, cy - 1.5f);
    chevron.lineTo (cx,        cy + 2.5f);
    chevron.lineTo (cx + 4.0f, cy - 1.5f);
    g.setColour (mutedText);
    g.strokePath (chevron, juce::PathStrokeType (1.5f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
}

void NSColourMapLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                              float sliderPos, float rotaryStartAngle, float rotaryEndAngle, juce::Slider&)
{
    const auto rawBounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (4.0f);
    const auto diameter  = juce::jmin (rawBounds.getWidth(), rawBounds.getHeight());
    const auto bounds    = juce::Rectangle<float> (diameter, diameter).withCentre (rawBounds.getCentre());
    const auto radius    = bounds.getWidth() * 0.5f;
    const auto centre    = bounds.getCentre();
    const auto angle     = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    g.setGradientFill (juce::ColourGradient (juce::Colour { 0xff3d444du }, centre.x, bounds.getY(),
                                             juce::Colour { 0xff171a1du }, centre.x, bounds.getBottom(), false));
    g.fillEllipse (bounds);
    g.setColour (juce::Colour { 0xff090a0cu });
    g.drawEllipse (bounds, 1.5f);

    juce::Path groove;
    groove.addCentredArc (centre.x, centre.y, radius - 5.0f, radius - 5.0f, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (juce::Colour { 0xff1b1e22u });
    g.strokePath (groove, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path arc;
    arc.addCentredArc (centre.x, centre.y, radius - 5.0f, radius - 5.0f, 0.0f, rotaryStartAngle, angle, true);
    g.setColour (accent);
    g.strokePath (arc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const auto markerStart = centre.getPointOnCircumference (radius * 0.22f, angle);
    const auto markerEnd   = centre.getPointOnCircumference (radius * 0.60f, angle);
    g.setColour (text.withAlpha (0.90f));
    g.drawLine ({ markerStart, markerEnd }, 2.0f);

    const auto thumbPt = centre.getPointOnCircumference (radius - 5.0f, angle);
    g.setColour (accent.brighter (0.3f));
    g.fillEllipse (thumbPt.x - 2.5f, thumbPt.y - 2.5f, 5.0f, 5.0f);
}

void NSColourMapLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                              bool highlighted, bool)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    const auto on     = button.getToggleState();
    const auto base   = on ? accent.withAlpha (0.22f)
                           : (highlighted ? panelLight.brighter (0.1f) : panelLight);

    g.setColour (base);
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (on ? accent : juce::Colour { 0xff46505au });
    g.drawRoundedRectangle (bounds, 4.0f, on ? 1.8f : 1.0f);

    const float tickSize = juce::jmin (bounds.getHeight() * 0.45f, 10.0f);
    const float tickX    = bounds.getX() + 8.0f;
    const float tickY    = bounds.getCentreY() - tickSize * 0.5f;
    g.setColour (on ? accent : mutedText);
    g.fillEllipse (tickX, tickY, tickSize, tickSize);

    g.setFont (sectionFont());
    g.setColour (on ? text : mutedText);
    auto textArea = bounds.toNearestInt();
    textArea.removeFromLeft ((int) (tickX - bounds.getX()) + (int) tickSize + 6);
    g.drawText (button.getButtonText(), textArea, juce::Justification::centredLeft, true);
}

// ── SpectrumMapView ───────────────────────────────────────────────────────────

SpectrumMapView::SpectrumMapView (NSColourMapAudioProcessor& p) : processor (p) { startTimerHz (30); }
SpectrumMapView::~SpectrumMapView() { stopTimer(); }

void SpectrumMapView::timerCallback()
{
    const float e = processor.getInputEnergy();
    energySmoothed += 0.3f * (e - energySmoothed);
    repaint();
}

void SpectrumMapView::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (panel);
    g.fillRoundedRectangle (bounds, 8.0f);
    g.setColour (juce::Colour { 0xff3a4149u });
    g.drawRoundedRectangle (bounds, 8.0f, 1.0f);

    const auto plot = bounds.reduced (14.0f, 12.0f);

    // Octave grid columns
    g.setColour (juce::Colour { 0x224f5962u });
    for (int i = 0; i <= 6; ++i)
    {
        const float xx = plot.getX() + plot.getWidth() * (float) i / 6.0f;
        g.drawVerticalLine (juce::roundToInt (xx), plot.getY(), plot.getBottom());
    }

    // Sub-protect zone (left band sized by crossover proportion of low range)
    const float subHz   = processor.getSubProtectHz();
    const float subProp = juce::jlimit (0.02f, 0.25f, (std::log (subHz / 30.0f) / std::log (2000.0f / 30.0f)));
    auto subZone = plot.withWidth (plot.getWidth() * subProp);
    g.setColour (hotAccent.withAlpha (0.10f));
    g.fillRect (subZone);
    g.setColour (hotAccent.withAlpha (0.35f));
    g.drawVerticalLine (juce::roundToInt (subZone.getRight()), plot.getY(), plot.getBottom());
    g.setFont (juce::Font (juce::FontOptions (9.0f)));
    g.setColour (hotAccent.withAlpha (0.7f));
    g.drawText ("SUB", subZone.toNearestInt().reduced (2), juce::Justification::topLeft);

    // Target pitch lanes spread over the C2..C7 range (MIDI 36..96)
    const auto mask = processor.getTargetMask();
    const float e   = juce::jlimit (0.0f, 1.0f, energySmoothed * 6.0f);
    if (mask != 0)
    {
        for (int note = 36; note <= 96; ++note)
        {
            const int pc = note % 12;
            if (! (mask & (1u << pc)))
                continue;
            const float norm = (float) (note - 36) / 60.0f;
            const float xx   = plot.getX() + norm * plot.getWidth();
            const float h    = plot.getHeight() * (0.3f + 0.6f * e);
            const auto lane  = juce::Rectangle<float> (xx - 1.5f, plot.getBottom() - h, 3.0f, h);
            g.setColour (accent.withAlpha (0.35f + 0.5f * e));
            g.fillRoundedRectangle (lane, 1.5f);
            g.setColour (accent.brighter (0.4f));
            g.fillEllipse (xx - 2.5f, plot.getBottom() - h - 2.5f, 5.0f, 5.0f);
        }
    }
    else
    {
        g.setFont (labelFont());
        g.setColour (mutedText.withAlpha (0.6f));
        g.drawText ("Send MIDI chords, or switch to Scale Resonance",
                    plot.toNearestInt(), juce::Justification::centred);
    }

    // Transient flash overlay
    const float flash = processor.getTransientFlash();
    if (flash > 0.01f)
    {
        g.setColour (hotAccent.withAlpha (0.18f * flash));
        g.fillRoundedRectangle (bounds, 8.0f);
    }

    // Label
    g.setFont (labelFont());
    g.setColour (mutedText);
    g.drawText ("Colour Spectrum Map", bounds.toNearestInt().reduced (14, 8), juce::Justification::topRight);
}

// ── KeyboardView ──────────────────────────────────────────────────────────────

KeyboardView::KeyboardView (NSColourMapAudioProcessor& p) : processor (p) { startTimerHz (20); }
KeyboardView::~KeyboardView() { stopTimer(); }

void KeyboardView::timerCallback()
{
    const auto t = processor.getTargetMask();
    const auto h = processor.getHeldMask();
    if (t != lastTarget || h != lastHeld)
    {
        lastTarget = t;
        lastHeld   = h;
        repaint();
    }
}

void KeyboardView::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (juce::Colour { 0xff1b1e22u });
    g.fillRoundedRectangle (bounds, 6.0f);

    const auto mask = processor.getTargetMask();
    const auto held = processor.getHeldMask();

    constexpr int octaves = 2;
    constexpr int whitePerOct = 7;
    const int whiteCount = octaves * whitePerOct;
    const float kbW = bounds.getWidth() - 8.0f;
    const float kbX = bounds.getX() + 4.0f;
    const float kbY = bounds.getY() + 3.0f;
    const float kbH = bounds.getHeight() - 6.0f;
    const float whiteW = kbW / (float) whiteCount;

    static constexpr int whitePcs[7] = { 0, 2, 4, 5, 7, 9, 11 };
    static constexpr int blackAfter[7] = { 1, 1, 0, 1, 1, 1, 0 }; // black key after this white?

    // White keys
    for (int i = 0; i < whiteCount; ++i)
    {
        const int pc = whitePcs[i % 7];
        const auto r = juce::Rectangle<float> (kbX + i * whiteW, kbY, whiteW - 1.0f, kbH);
        const bool isTarget = (mask & (1u << pc)) != 0;
        const bool isHeld   = (held & (1u << pc)) != 0;
        g.setColour (isTarget ? accent.withAlpha (0.85f) : juce::Colour { 0xffd8dee4u });
        g.fillRoundedRectangle (r, 2.0f);
        if (isHeld)
        {
            g.setColour (hotAccent);
            g.drawRoundedRectangle (r.reduced (0.5f), 2.0f, 1.6f);
        }
    }

    // Black keys
    const float blackW = whiteW * 0.62f;
    const float blackH = kbH * 0.6f;
    for (int i = 0; i < whiteCount; ++i)
    {
        if (! blackAfter[i % 7])
            continue;
        const int pc = (whitePcs[i % 7] + 1) % 12;
        const float bx = kbX + (i + 1) * whiteW - blackW * 0.5f;
        const auto r = juce::Rectangle<float> (bx, kbY, blackW, blackH);
        const bool isTarget = (mask & (1u << pc)) != 0;
        const bool isHeld   = (held & (1u << pc)) != 0;
        g.setColour (isTarget ? accent.darker (0.2f) : juce::Colour { 0xff15171au });
        g.fillRoundedRectangle (r, 2.0f);
        if (isHeld)
        {
            g.setColour (hotAccent);
            g.drawRoundedRectangle (r.reduced (0.5f), 2.0f, 1.4f);
        }
    }
}

// ── Editor ────────────────────────────────────────────────────────────────────

NSColourMapAudioProcessorEditor::NSColourMapAudioProcessorEditor (NSColourMapAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), spectrum (p), keyboard (p)
{
    setLookAndFeel (&lnf);
    setResizable (true, true);
    setResizeLimits (560, 520, 1080, 900);
    setSize (720, 640);

    logoLabel.setText ("NSColourMap", juce::dontSendNotification);
    logoLabel.setFont (titleFont());
    logoLabel.setColour (juce::Label::textColourId, text);
    addAndMakeVisible (logoLabel);

    configureTab (mainTab, 0);
    configureTab (aboutTab, 1);

    midiIndicator.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (midiIndicator);

    statusLabel.setFont (sectionFont());
    statusLabel.setColour (juce::Label::textColourId, mutedText);
    statusLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (statusLabel);

    addAndMakeVisible (spectrum);
    addAndMakeVisible (keyboard);

    // Mode radios
    configureRadio (chordModeBtn, nscm::params::mode, 0, 1001);
    configureRadio (scaleModeBtn, nscm::params::mode, 1, 1001);

    // Algo radios
    for (int i = 0; i < (int) algoButtons.size(); ++i)
        configureRadio (algoButtons[(size_t) i], nscm::params::algo, i, 1002);

    // Key / Scale / Quality combos
    keyBox.addItemList (nscm::params::keyNames(), 1);
    scaleBox.addItemList (nscm::params::scaleNames(), 1);
    qualityBox.addItemList (nscm::params::qualityNames(), 1);
    addAndMakeVisible (keyBox);
    addAndMakeVisible (scaleBox);
    addAndMakeVisible (qualityBox);

    auto initLabel = [this] (juce::Label& l, const juce::String& t)
    {
        l.setText (t, juce::dontSendNotification);
        l.setFont (labelFont());
        l.setColour (juce::Label::textColourId, mutedText);
        addAndMakeVisible (l);
    };
    initLabel (keyLabel, "Key");
    initLabel (scaleLabel, "Scale");
    initLabel (qualityLabel, "Quality");
    initLabel (scaleShiftLabel, "Shift");

    addAndMakeVisible (freezeButton);

    scaleShiftSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    scaleShiftSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 16);
    addAndMakeVisible (scaleShiftSlider);

    configureKnob (amountKnob, amountLabel, "Amount");
    configureKnob (glideKnob, glideLabel, "Glide");
    configureKnob (colourKnob, colourLabel, "Colour");
    configureKnob (formantKnob, formantLabel, "Formant");
    configureKnob (subKnob, subLabel, "Sub Protect");
    configureKnob (transientKnob, transientLabel, "Transient");
    configureKnob (mixKnob, mixLabel, "Mix");
    configureKnob (outputKnob, outputLabel, "Output");

    // Preset selector (header)
    presetBox.setTextWhenNothingSelected ("Presets");
    presetBox.setJustificationType (juce::Justification::centredLeft);
    for (int i = 0; i < (int) nscm::presets::factory.size(); ++i)
        presetBox.addItem (nscm::presets::factory[(size_t) i].name, i + 1);
    presetBox.onChange = [this]
    {
        const int idx = presetBox.getSelectedItemIndex();
        if (idx >= 0 && idx < (int) nscm::presets::factory.size())
            nscm::presets::apply (audioProcessor.getState(), nscm::presets::factory[(size_t) idx]);
    };
    addAndMakeVisible (presetBox);

    // Snapshots A/B/C/D — click recalls (or stores if empty), Alt-click overwrites.
    for (int i = 0; i < (int) snapshots.size(); ++i)
    {
        auto& s = snapshots[(size_t) i];
        s.setClickingTogglesState (false);
        s.setTooltip ("Snapshot " + juce::String ("ABCD").substring (i, i + 1)
                      + " — click: recall / store · Alt-click: overwrite");
        s.onClick = [this, i] { handleSnapshot (i); };
        addAndMakeVisible (s);
    }

    // Attachments
    auto& state = audioProcessor.getState();
    amountAtt     = std::make_unique<SliderAttachment> (state, nscm::params::amount,     amountKnob);
    glideAtt      = std::make_unique<SliderAttachment> (state, nscm::params::glide,      glideKnob);
    colourAtt     = std::make_unique<SliderAttachment> (state, nscm::params::colour,     colourKnob);
    formantAtt    = std::make_unique<SliderAttachment> (state, nscm::params::formant,    formantKnob);
    subAtt        = std::make_unique<SliderAttachment> (state, nscm::params::subProtect, subKnob);
    transientAtt  = std::make_unique<SliderAttachment> (state, nscm::params::transient,  transientKnob);
    mixAtt        = std::make_unique<SliderAttachment> (state, nscm::params::mix,        mixKnob);
    outputAtt     = std::make_unique<SliderAttachment> (state, nscm::params::output,     outputKnob);
    scaleShiftAtt = std::make_unique<SliderAttachment> (state, nscm::params::scaleShift, scaleShiftSlider);
    keyAtt        = std::make_unique<ComboBoxAttachment> (state, nscm::params::key,      keyBox);
    scaleAtt      = std::make_unique<ComboBoxAttachment> (state, nscm::params::scale,    scaleBox);
    qualityAtt    = std::make_unique<ComboBoxAttachment> (state, nscm::params::quality,  qualityBox);
    freezeAtt     = std::make_unique<ButtonAttachment>   (state, nscm::params::freezeChord, freezeButton);

    currentTab = choiceIndex (state, nscm::params::uiTab);
    setCurrentTab (currentTab);
    startTimerHz (20);
}

NSColourMapAudioProcessorEditor::~NSColourMapAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void NSColourMapAudioProcessorEditor::configureKnob (juce::Slider& slider, juce::Label& label, const juce::String& labelText)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 18);
    slider.setColour (juce::Slider::textBoxTextColourId,       text);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour { 0x00000000u });
    slider.setColour (juce::Slider::textBoxOutlineColourId,    juce::Colour { 0x00000000u });
    addAndMakeVisible (slider);

    label.setText (labelText, juce::dontSendNotification);
    label.setFont (labelFont());
    label.setColour (juce::Label::textColourId, mutedText);
    label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (label);
}

void NSColourMapAudioProcessorEditor::configureTab (juce::TextButton& button, int index)
{
    button.setClickingTogglesState (false);
    button.onClick = [this, index] { setCurrentTab (index); };
    addAndMakeVisible (button);
}

void NSColourMapAudioProcessorEditor::configureRadio (juce::TextButton& button, const char* paramId, int valueIndex, int)
{
    button.setClickingTogglesState (false);
    button.onClick = [this, paramId, valueIndex] { setChoice (audioProcessor.getState(), paramId, valueIndex); syncRadios(); };
    addAndMakeVisible (button);
}

void NSColourMapAudioProcessorEditor::setCurrentTab (int index)
{
    currentTab = juce::jlimit (0, 1, index);
    setChoice (audioProcessor.getState(), nscm::params::uiTab, currentTab);
    mainTab.setToggleState  (currentTab == 0, juce::dontSendNotification);
    aboutTab.setToggleState (currentTab == 1, juce::dontSendNotification);

    const bool main = currentTab == 0;
    spectrum.setVisible (main);
    keyboard.setVisible (main);
    chordModeBtn.setVisible (main);
    scaleModeBtn.setVisible (main);
    for (auto& b : algoButtons) b.setVisible (main);
    keyBox.setVisible (main); scaleBox.setVisible (main); qualityBox.setVisible (main);
    keyLabel.setVisible (main); scaleLabel.setVisible (main); qualityLabel.setVisible (main); scaleShiftLabel.setVisible (main);
    freezeButton.setVisible (main); scaleShiftSlider.setVisible (main);
    statusLabel.setVisible (main);
    for (auto* s : { &amountKnob, &glideKnob, &colourKnob, &formantKnob, &subKnob, &transientKnob, &mixKnob, &outputKnob })
        s->setVisible (main);
    for (auto* l : { &amountLabel, &glideLabel, &colourLabel, &formantLabel, &subLabel, &transientLabel, &mixLabel, &outputLabel })
        l->setVisible (main);
    for (auto& s : snapshots) s.setVisible (main);

    resized();
    repaint();
}

void NSColourMapAudioProcessorEditor::syncRadios()
{
    auto& state = audioProcessor.getState();
    const int mode = choiceIndex (state, nscm::params::mode);
    chordModeBtn.setToggleState (mode == 0, juce::dontSendNotification);
    scaleModeBtn.setToggleState (mode == 1, juce::dontSendNotification);

    const int algo = choiceIndex (state, nscm::params::algo);
    for (int i = 0; i < (int) algoButtons.size(); ++i)
        algoButtons[(size_t) i].setToggleState (i == algo, juce::dontSendNotification);
}

void NSColourMapAudioProcessorEditor::handleSnapshot (int index)
{
    const bool overwrite = juce::ModifierKeys::getCurrentModifiers().isAltDown();
    if (overwrite || ! audioProcessor.isSnapshotFilled (index))
        audioProcessor.storeSnapshot (index);
    else
        audioProcessor.recallSnapshot (index);
}

void NSColourMapAudioProcessorEditor::timerCallback()
{
    syncRadios();
    repaint (midiIndicator.getBounds().expanded (8));

    for (int i = 0; i < (int) snapshots.size(); ++i)
        snapshots[(size_t) i].setToggleState (audioProcessor.isSnapshotFilled (i), juce::dontSendNotification);

    if (currentTab == 0)
    {
        const auto mask = audioProcessor.getTargetMask();
        const int mode  = choiceIndex (audioProcessor.getState(), nscm::params::mode);
        juce::String s;
        if (mode == 0)
        {
            s = "Chord: ";
            if (mask == 0) s += "—";
            else for (int pc = 0; pc < 12; ++pc) if (mask & (1u << pc)) s += juce::String (kNoteNames[pc]) + " ";
        }
        else
        {
            const int key   = choiceIndex (audioProcessor.getState(), nscm::params::key);
            const int scale = choiceIndex (audioProcessor.getState(), nscm::params::scale);
            s = juce::String ("Scale: ") + kNoteNames[key % 12] + " "
                + nscm::getScaleName ((nscm::Scale) scale);
        }
        s += "  |  " + juce::String (audioProcessor.getActiveVoiceCount()) + " voices";
        statusLabel.setText (s, juce::dontSendNotification);
    }
}

void NSColourMapAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (background);

    auto bounds = getLocalBounds();
    g.setColour (juce::Colour { 0xff0f1113u });
    g.fillRect (bounds.removeFromTop (48));
    g.setColour (juce::Colour { 0xff000000u });
    g.drawHorizontalLine (48, 0.0f, (float) getWidth());
    g.setColour (juce::Colour { 0xff38404au });
    g.drawHorizontalLine (49, 0.0f, (float) getWidth());

    // MIDI LED
    const auto level = audioProcessor.getMidiActivity();
    const auto ib    = midiIndicator.getBounds().toFloat();
    const bool active = level > 0.0f;
    if (active)
    {
        g.setColour (hotAccent.withAlpha (0.10f * level));
        g.fillEllipse (ib.expanded (6.0f));
        g.setColour (hotAccent.withAlpha (0.22f * level));
        g.fillEllipse (ib.expanded (3.0f));
    }
    const auto led = active ? hotAccent : juce::Colour { 0xff3a3e42u };
    g.setGradientFill (juce::ColourGradient (led.brighter (active ? 0.35f * level : 0.0f),
                                             ib.getCentreX(), ib.getY(),
                                             led.darker (0.2f), ib.getCentreX(), ib.getBottom(), false));
    g.fillEllipse (ib);
    g.setColour (juce::Colours::white.withAlpha (0.18f));
    g.fillEllipse (ib.reduced (3.0f).translated (0.0f, -1.0f));

    // Section labels on main page
    if (currentTab == 0)
    {
        g.setFont (sectionFont());
        g.setColour (mutedText.withAlpha (0.5f));
    }

    // About page
    if (currentTab == 1)
    {
        auto area = getLocalBounds();
        area.removeFromTop (54);
        area.reduce (36, 24);
        g.setColour (panel);
        g.fillRoundedRectangle (getLocalBounds().reduced (22).withTrimmedTop (32).toFloat(), 8.0f);

        g.setFont (titleFont());
        g.setColour (text);
        g.drawText ("NSColourMap", area.removeFromTop (40), juce::Justification::centred);

        const auto badgeW = 64.0f, badgeH = 22.0f;
        const auto badge  = juce::Rectangle<float> ((float) getWidth() * 0.5f - badgeW * 0.5f,
                                                    (float) area.getY() + 1.0f, badgeW, badgeH);
        g.setColour (accent.withAlpha (0.15f));
        g.fillRoundedRectangle (badge, 5.0f);
        g.setColour (accent);
        g.setFont (sectionFont());
        g.drawText ("v0.3.0", badge.toNearestInt(), juce::Justification::centred);
        area.removeFromTop (34);

        g.setColour (panelLight.brighter (0.1f));
        g.fillRect (juce::Rectangle<float> ((float) getWidth() * 0.32f, (float) area.getY(), (float) getWidth() * 0.36f, 1.0f));
        area.removeFromTop (16);

        g.setFont (juce::Font (juce::FontOptions{}.withHeight (14.0f)));
        g.setColour (mutedText);
        g.drawText ("MIDI / scale-aware harmonic colour for Colour Bass & HiTECH",
                    area.removeFromTop (22), juce::Justification::centred, true);
        g.setFont (juce::Font (juce::FontOptions{}.withHeight (13.0f)));
        g.setColour (text.withAlpha (0.65f));
        g.drawText ("by nisesimadao", area.removeFromTop (22), juce::Justification::centred);
        area.removeFromTop (18);

        g.setFont (juce::Font (juce::FontOptions{}.withHeight (12.0f)));
        g.setColour (mutedText.withAlpha (0.75f));
        g.drawText ("Resonator Bank · Sub Protect · Transient Bypass · Pseudo Formant",
                    area.removeFromTop (20), juce::Justification::centred, true);
        area.removeFromTop (24);

        g.setFont (juce::Font (juce::FontOptions{}.withHeight (11.0f)));
        g.setColour (mutedText.withAlpha (0.45f));
        g.drawText ("Built with JUCE 8  |  Free / MIT", area.removeFromTop (18), juce::Justification::centred);
    }
}

void NSColourMapAudioProcessorEditor::resized()
{
    auto area   = getLocalBounds();
    auto header = area.removeFromTop (48).reduced (14, 6);
    logoLabel.setBounds (header.removeFromLeft (158));
    aboutTab.setBounds (header.removeFromRight (72));
    mainTab.setBounds (header.removeFromRight (70).reduced (4, 0));
    const auto indicatorSlot = header.removeFromRight (20);
    midiIndicator.setBounds (indicatorSlot.getCentreX() - 6, indicatorSlot.getCentreY() - 6, 12, 12);
    header.removeFromLeft (6);
    presetBox.setBounds (header.removeFromLeft (juce::jmin (200, header.getWidth())).reduced (0, 8));

    if (currentTab != 0)
        return;

    area.reduce (16, 12);

    // Bottom-up stack
    auto snapsRow = area.removeFromBottom (24);
    {
        auto r = snapsRow.removeFromRight (200);
        const int w = r.getWidth() / 4;
        for (int i = 0; i < 4; ++i)
            snapshots[(size_t) i].setBounds (r.removeFromLeft (w).reduced (3, 1));
    }
    area.removeFromBottom (6);

    auto utilRow  = area.removeFromBottom (96);
    auto macroRow = area.removeFromBottom (96);

    auto placeKnobRow = [] (juce::Rectangle<int> row, std::array<juce::Slider*, 4> knobs, std::array<juce::Label*, 4> labels)
    {
        const int w = row.getWidth() / 4;
        for (int i = 0; i < 4; ++i)
        {
            auto col = row.removeFromLeft (i == 3 ? row.getWidth() : w).reduced (4, 0);
            labels[(size_t) i]->setBounds (col.removeFromTop (16));
            knobs[(size_t) i]->setBounds (col);
        }
    };
    placeKnobRow (macroRow, { &amountKnob, &glideKnob, &colourKnob, &formantKnob },
                            { &amountLabel, &glideLabel, &colourLabel, &formantLabel });
    placeKnobRow (utilRow,  { &subKnob, &transientKnob, &mixKnob, &outputKnob },
                            { &subLabel, &transientLabel, &mixLabel, &outputLabel });

    // Settings row: Key / Scale / Shift / Quality / Freeze
    auto settings = area.removeFromBottom (44);
    {
        const int colW = juce::jmax (70, settings.getWidth() / 5);
        auto place = [&] (juce::Label& l, juce::Component& c)
        {
            auto col = settings.removeFromLeft (colW).reduced (4, 2);
            l.setBounds (col.removeFromTop (16));
            c.setBounds (col.removeFromTop (24));
        };
        place (keyLabel, keyBox);
        place (scaleLabel, scaleBox);
        place (scaleShiftLabel, scaleShiftSlider);
        place (qualityLabel, qualityBox);
        auto col = settings.reduced (4, 2);
        col.removeFromTop (16);
        freezeButton.setBounds (col.removeFromTop (24));
    }
    area.removeFromBottom (6);

    // Algo row
    auto algoRow = area.removeFromBottom (28);
    {
        const int w = algoRow.getWidth() / (int) algoButtons.size();
        for (int i = 0; i < (int) algoButtons.size(); ++i)
            algoButtons[(size_t) i].setBounds (algoRow.removeFromLeft (i == (int) algoButtons.size() - 1 ? algoRow.getWidth() : w).reduced (2, 2));
    }
    // Mode row
    auto modeRow = area.removeFromBottom (28);
    {
        const int w = modeRow.getWidth() / 2;
        chordModeBtn.setBounds (modeRow.removeFromLeft (w).reduced (2, 2));
        scaleModeBtn.setBounds (modeRow.reduced (2, 2));
    }
    area.removeFromBottom (6);

    // Keyboard
    keyboard.setBounds (area.removeFromBottom (60));
    area.removeFromBottom (6);

    // Status row
    statusLabel.setBounds (area.removeFromTop (20));
    area.removeFromTop (2);

    // Spectrum fills the rest
    spectrum.setBounds (area);
}
