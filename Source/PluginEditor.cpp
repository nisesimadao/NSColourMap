#include "PluginEditor.h"

#include "Parameters.h"
#include "Presets.h"
#include "dsp/CharacterModes.h"
#include "dsp/ScaleNoteSet.h"

#include <cmath>

namespace
{
const juce::Colour background  { 0xff15171a };
const juce::Colour panel       { 0xff24282d };
const juce::Colour panelLight  { 0xff30363d };
const juce::Colour text        { 0xfff2f5f8 };
const juce::Colour mutedText   { 0xff9aa4ad };
const juce::Colour accent      { 0xff38c7f3 }; // TUNED (cyan)
const juce::Colour hotAccent   { 0xffff715b }; // transient / MIDI
const juce::Colour dryCol      { 0xff8b7bf0 }; // DRY (purple)
const juce::Colour coloredCol  { 0xffffc24b }; // COLORED (amber)
const juce::Colour protectedCol{ 0xff4a525c }; // PROTECTED (grey)

#if JUCE_WINDOWS
constexpr const char* kSansRegular = "Segoe UI";
constexpr const char* kSansLight   = "Segoe UI Light";
#else
constexpr const char* kSansRegular = "Helvetica Neue";
constexpr const char* kSansLight   = "Helvetica Neue";
#endif

juce::Font labelFont()   { return juce::Font (juce::FontOptions{}.withName (kSansLight).withHeight (12.0f).withStyle ("Light")); }
juce::Font titleFont()   { return juce::Font (juce::FontOptions{}.withName (kSansLight).withHeight (21.0f).withStyle ("Light")); }
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

float midiHz (float note) { return 440.0f * std::pow (2.0f, (note - 69.0f) / 12.0f); }
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
                                                   const juce::Colour&, bool highlighted, bool)
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
            juce::Rectangle<float> (bounds.getX() + 7.0f, bounds.getBottom() - 3.0f,
                                    bounds.getWidth() - 14.0f, 2.5f), 1.2f);
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
    const float arcThick = juce::jmax (3.0f, radius * 0.11f);

    g.setGradientFill (juce::ColourGradient (juce::Colour { 0xff3d444du }, centre.x, bounds.getY(),
                                             juce::Colour { 0xff171a1du }, centre.x, bounds.getBottom(), false));
    g.fillEllipse (bounds);
    g.setColour (juce::Colour { 0xff090a0cu });
    g.drawEllipse (bounds, 1.5f);

    juce::Path groove;
    groove.addCentredArc (centre.x, centre.y, radius - arcThick - 1.0f, radius - arcThick - 1.0f, 0.0f,
                          rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (juce::Colour { 0xff1b1e22u });
    g.strokePath (groove, juce::PathStrokeType (arcThick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path arc;
    arc.addCentredArc (centre.x, centre.y, radius - arcThick - 1.0f, radius - arcThick - 1.0f, 0.0f,
                       rotaryStartAngle, angle, true);
    g.setColour (accent);
    g.strokePath (arc, juce::PathStrokeType (arcThick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const auto markerStart = centre.getPointOnCircumference (radius * 0.22f, angle);
    const auto markerEnd   = centre.getPointOnCircumference (radius * 0.58f, angle);
    g.setColour (text.withAlpha (0.90f));
    g.drawLine ({ markerStart, markerEnd }, juce::jmax (2.0f, radius * 0.06f));

    const auto thumbPt = centre.getPointOnCircumference (radius - arcThick - 1.0f, angle);
    const float dot = juce::jmax (3.0f, radius * 0.08f);
    g.setColour (accent.brighter (0.3f));
    g.fillEllipse (thumbPt.x - dot, thumbPt.y - dot, dot * 2.0f, dot * 2.0f);
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

    const float tickSize = juce::jmin (bounds.getHeight() * 0.42f, 9.0f);
    const float tickX    = bounds.getX() + 7.0f;
    const float tickY    = bounds.getCentreY() - tickSize * 0.5f;
    g.setColour (on ? accent : mutedText);
    g.fillEllipse (tickX, tickY, tickSize, tickSize);

    g.setFont (sectionFont());
    g.setColour (on ? text : mutedText);
    auto textArea = bounds.toNearestInt();
    textArea.removeFromLeft ((int) (tickX - bounds.getX()) + (int) tickSize + 5);
    g.drawText (button.getButtonText(), textArea, juce::Justification::centredLeft, true);
}

// ── VisualizerView ────────────────────────────────────────────────────────────

VisualizerView::VisualizerView (NSColourMapAudioProcessor& p) : processor (p) { startTimerHz (30); }
VisualizerView::~VisualizerView() { stopTimer(); }

void VisualizerView::timerCallback()
{
    inSm    += 0.3f * (processor.getInputEnergy()   - inSm);
    tunedSm += 0.3f * (processor.getTunedEnergy()   - tunedSm);
    colSm   += 0.3f * (processor.getColoredEnergy() - colSm);
    repaint();
}

float VisualizerView::freqToX (float hz, juce::Rectangle<float> plot) const
{
    constexpr float fLo = 40.0f, fHi = 12000.0f;
    const float t = std::log (juce::jlimit (fLo, fHi, hz) / fLo) / std::log (fHi / fLo);
    return plot.getX() + t * plot.getWidth();
}

void VisualizerView::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (panel);
    g.fillRoundedRectangle (bounds, 8.0f);
    g.setColour (juce::Colour { 0xff3a4149u });
    g.drawRoundedRectangle (bounds, 8.0f, 1.0f);

    const auto plot = bounds.reduced (14.0f, 14.0f).withTrimmedTop (6.0f);

    const float lowHz  = processor.getLowHz();
    const float highHz = processor.getHighHz();
    const float xLow   = freqToX (lowHz, plot);
    const float xHigh  = freqToX (highHz, plot);

    // Protected zones (outside the affected range)
    g.setColour (protectedCol.withAlpha (0.16f));
    g.fillRect (juce::Rectangle<float> (plot.getX(), plot.getY(), xLow - plot.getX(), plot.getHeight()));
    g.fillRect (juce::Rectangle<float> (xHigh, plot.getY(), plot.getRight() - xHigh, plot.getHeight()));
    g.setColour (protectedCol.withAlpha (0.5f));
    g.drawVerticalLine (juce::roundToInt (xLow),  plot.getY(), plot.getBottom());
    g.drawVerticalLine (juce::roundToInt (xHigh), plot.getY(), plot.getBottom());

    // DRY baseline energy (purple)
    const float inLvl = juce::jlimit (0.0f, 1.0f, inSm * 4.0f);
    const float dryH  = plot.getHeight() * (0.06f + 0.30f * inLvl);
    g.setColour (dryCol.withAlpha (0.22f));
    g.fillRect (juce::Rectangle<float> (plot.getX(), plot.getBottom() - dryH, plot.getWidth(), dryH));

    // TUNED lanes (cyan) + COLORED tips (amber)
    const auto mask  = processor.getTargetMask();
    const auto held  = processor.getHeldMask();
    const float tLvl = juce::jlimit (0.05f, 1.0f, tunedSm * 5.0f);
    const float cLvl = juce::jlimit (0.0f, 1.0f, colSm * 5.0f);

    if (mask != 0)
    {
        for (int note = 24; note <= 108; ++note)
        {
            const int pc = note % 12;
            if (! (mask & (1u << pc)))
                continue;
            const float hz = midiHz ((float) note);
            if (hz < 40.0f || hz > 12000.0f)
                continue;
            const float xx = freqToX (hz, plot);
            const bool inRange = hz >= lowHz && hz <= highHz;
            const float h = plot.getHeight() * (0.25f + 0.65f * tLvl) * (inRange ? 1.0f : 0.35f);
            const auto lane = juce::Rectangle<float> (xx - 1.5f, plot.getBottom() - h, 3.0f, h);
            g.setColour (accent.withAlpha (inRange ? 0.30f + 0.55f * tLvl : 0.15f));
            g.fillRoundedRectangle (lane, 1.5f);

            if (inRange && cLvl > 0.01f)
            {
                const float ch = h * (0.25f + 0.5f * cLvl);
                g.setColour (coloredCol.withAlpha (0.35f + 0.5f * cLvl));
                g.fillRoundedRectangle (juce::Rectangle<float> (xx - 1.5f, plot.getBottom() - h - ch, 3.0f, ch), 1.5f);
            }

            g.setColour ((inRange ? accent : protectedCol).brighter (0.3f));
            g.fillEllipse (xx - 2.0f, plot.getBottom() - h - 2.0f, 4.0f, 4.0f);

            if (held & (1u << pc)) // MIDI active marker
            {
                g.setColour (hotAccent);
                g.fillEllipse (xx - 2.5f, plot.getBottom() - 5.0f, 5.0f, 5.0f);
            }
        }
    }
    else
    {
        g.setFont (labelFont());
        g.setColour (mutedText.withAlpha (0.6f));
        g.drawText ("Pick a Key/Scale or send MIDI chords, then turn COLOR",
                    plot.toNearestInt(), juce::Justification::centred);
    }

    // Transient flash
    const float flash = processor.getTransientFlash();
    if (flash > 0.01f)
    {
        g.setColour (hotAccent.withAlpha (0.16f * flash));
        g.fillRoundedRectangle (bounds, 8.0f);
    }

    // Legend
    g.setFont (juce::Font (juce::FontOptions (10.0f)));
    struct LegendItem { const char* label; juce::Colour col; int w; };
    const LegendItem items[] = { { "DRY", dryCol, 28 }, { "TUNED", accent, 40 },
                                 { "COLORED", coloredCol, 52 }, { "PROTECTED", protectedCol, 62 } };
    float lx = bounds.getX() + 14.0f;
    const float ly = bounds.getY() + 8.0f;
    for (auto& it : items)
    {
        g.setColour (it.col);
        g.fillRoundedRectangle (lx, ly + 2.0f, 8.0f, 8.0f, 2.0f);
        g.setColour (mutedText);
        g.drawText (it.label, juce::Rectangle<int> ((int) lx + 11, (int) ly, it.w, 12), juce::Justification::centredLeft);
        lx += 11.0f + (float) it.w + 8.0f;
    }
}

// ── KeyboardView ──────────────────────────────────────────────────────────────

KeyboardView::KeyboardView (NSColourMapAudioProcessor& p) : processor (p) { startTimerHz (20); }
KeyboardView::~KeyboardView() { stopTimer(); }

void KeyboardView::timerCallback()
{
    const auto t = processor.getTargetMask();
    const auto h = processor.getHeldMask();
    const int  r = choiceIndex (processor.getState(), nscm::params::key);
    if (t != lastTarget || h != lastHeld || r != lastRoot)
    {
        lastTarget = t; lastHeld = h; lastRoot = r;
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
    const int  root = choiceIndex (processor.getState(), nscm::params::key);

    constexpr int octaves = 2;
    const int whiteCount = octaves * 7;
    const float kbX = bounds.getX() + 4.0f, kbY = bounds.getY() + 3.0f;
    const float kbW = bounds.getWidth() - 8.0f, kbH = bounds.getHeight() - 6.0f;
    const float whiteW = kbW / (float) whiteCount;

    static constexpr int whitePcs[7]  = { 0, 2, 4, 5, 7, 9, 11 };
    static constexpr int blackAfter[7]= { 1, 1, 0, 1, 1, 1, 0 };

    for (int i = 0; i < whiteCount; ++i)
    {
        const int pc = whitePcs[i % 7];
        const auto r = juce::Rectangle<float> (kbX + i * whiteW, kbY, whiteW - 1.0f, kbH);
        const bool isTarget = (mask & (1u << pc)) != 0;
        const bool isRoot   = (pc == root);
        g.setColour (isTarget ? accent.withAlpha (0.85f) : juce::Colour { 0xffd8dee4u });
        g.fillRoundedRectangle (r, 2.0f);
        if (isRoot)
        {
            g.setColour (coloredCol);
            g.drawRoundedRectangle (r.reduced (0.5f), 2.0f, 2.0f);
        }
        if (held & (1u << pc))
        {
            g.setColour (hotAccent);
            g.fillRoundedRectangle (juce::Rectangle<float> (r.getX() + 1.0f, r.getBottom() - 5.0f,
                                                            r.getWidth() - 2.0f, 4.0f), 1.0f);
        }
    }

    const float blackW = whiteW * 0.62f, blackH = kbH * 0.6f;
    for (int i = 0; i < whiteCount; ++i)
    {
        if (! blackAfter[i % 7])
            continue;
        const int pc = (whitePcs[i % 7] + 1) % 12;
        const float bx = kbX + (i + 1) * whiteW - blackW * 0.5f;
        const auto r = juce::Rectangle<float> (bx, kbY, blackW, blackH);
        const bool isTarget = (mask & (1u << pc)) != 0;
        g.setColour (isTarget ? accent.darker (0.2f) : juce::Colour { 0xff15171au });
        g.fillRoundedRectangle (r, 2.0f);
        if (pc == root)
        {
            g.setColour (coloredCol);
            g.drawRoundedRectangle (r.reduced (0.5f), 2.0f, 1.4f);
        }
        if (held & (1u << pc))
        {
            g.setColour (hotAccent);
            g.drawRoundedRectangle (r.reduced (0.5f), 2.0f, 1.2f);
        }
    }
}

// ── Editor ────────────────────────────────────────────────────────────────────

NSColourMapAudioProcessorEditor::NSColourMapAudioProcessorEditor (NSColourMapAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), visualizer (p), keyboard (p)
{
    setLookAndFeel (&lnf);
    setResizable (true, true);
    setResizeLimits (600, 560, 1180, 980);
    setSize (760, 660);

    logoLabel.setText ("NSColourMap", juce::dontSendNotification);
    logoLabel.setFont (titleFont());
    logoLabel.setColour (juce::Label::textColourId, text);
    addAndMakeVisible (logoLabel);

    // Preset
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

    midiIndicator.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (midiIndicator);

    qualityButton.setClickingTogglesState (false);
    qualityButton.setTooltip ("Quality: 0 Latency / High Quality");
    qualityButton.onClick = [this]
    {
        const int q = choiceIndex (audioProcessor.getState(), nscm::params::quality);
        setChoice (audioProcessor.getState(), nscm::params::quality, q == 0 ? 1 : 0);
    };
    addAndMakeVisible (qualityButton);

    advancedButton.setClickingTogglesState (false);
    advancedButton.setTooltip ("Show advanced controls");
    advancedButton.onClick = [this] { showAdvanced = ! showAdvanced; advancedButton.setToggleState (showAdvanced, juce::dontSendNotification); updateMainVisibility(); resized(); };
    addAndMakeVisible (advancedButton);

    configureTab (mainTab, 0);
    configureTab (aboutTab, 1);

    // Grid mode + character radios
    for (int i = 0; i < (int) gridModeButtons.size(); ++i)
        configureRadio (gridModeButtons[(size_t) i], nscm::params::mode, i);
    for (int i = 0; i < (int) characterButtons.size(); ++i)
        configureRadio (characterButtons[(size_t) i], nscm::params::character, i);

    keyBox.addItemList (nscm::params::keyNames(), 1);
    scaleBox.addItemList (nscm::params::scaleNames(), 1);
    addAndMakeVisible (keyBox);
    addAndMakeVisible (scaleBox);
    addAndMakeVisible (freezeButton);

    addAndMakeVisible (visualizer);
    addAndMakeVisible (keyboard);

    configureKnob (colorKnob, colorLabel, "COLOR");
    colorKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 84, 20);
    configureKnob (amountKnob, amountLabel, "Amount");
    configureKnob (formantKnob, formantLabel, "Formant");
    configureKnob (transientKnob, transientLabel, "Transient");
    configureKnob (mixKnob, mixLabel, "Mix");
    configureKnob (outputKnob, outputLabel, "Output");

    configureKnob (gammaKnob, gammaLabel, "Gamma");
    configureKnob (morphKnob, morphLabel, "Morph");
    configureKnob (gateKnob, gateLabel, "Gate");
    configureKnob (lowCutKnob, lowCutLabel, "Low Cut");
    configureKnob (highCutKnob, highCutLabel, "High Cut");
    addAndMakeVisible (sideMuteButton);
    addAndMakeVisible (multirateButton);

    scaleShiftKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    scaleShiftKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 16);
    scaleShiftKnob.setColour (juce::Slider::textBoxTextColourId, text);
    scaleShiftKnob.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour { 0x00000000u });
    scaleShiftKnob.setColour (juce::Slider::textBoxOutlineColourId, juce::Colour { 0x00000000u });
    addAndMakeVisible (scaleShiftKnob);
    scaleShiftLabel.setText ("Shift", juce::dontSendNotification);
    scaleShiftLabel.setFont (labelFont());
    scaleShiftLabel.setColour (juce::Label::textColourId, mutedText);
    scaleShiftLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (scaleShiftLabel);

    // Attachments
    auto& s = audioProcessor.getState();
    colorAtt      = std::make_unique<SliderAttachment> (s, nscm::params::color,      colorKnob);
    amountAtt     = std::make_unique<SliderAttachment> (s, nscm::params::amount,     amountKnob);
    formantAtt    = std::make_unique<SliderAttachment> (s, nscm::params::formant,    formantKnob);
    transientAtt  = std::make_unique<SliderAttachment> (s, nscm::params::transient,  transientKnob);
    mixAtt        = std::make_unique<SliderAttachment> (s, nscm::params::mix,        mixKnob);
    outputAtt     = std::make_unique<SliderAttachment> (s, nscm::params::output,     outputKnob);
    gammaAtt      = std::make_unique<SliderAttachment> (s, nscm::params::gamma,      gammaKnob);
    morphAtt      = std::make_unique<SliderAttachment> (s, nscm::params::morph,      morphKnob);
    gateAtt       = std::make_unique<SliderAttachment> (s, nscm::params::gate,       gateKnob);
    lowCutAtt     = std::make_unique<SliderAttachment> (s, nscm::params::lowCut,     lowCutKnob);
    highCutAtt    = std::make_unique<SliderAttachment> (s, nscm::params::highCut,    highCutKnob);
    scaleShiftAtt = std::make_unique<SliderAttachment> (s, nscm::params::scaleShift, scaleShiftKnob);
    keyAtt        = std::make_unique<ComboBoxAttachment> (s, nscm::params::key,      keyBox);
    scaleAtt      = std::make_unique<ComboBoxAttachment> (s, nscm::params::scale,    scaleBox);
    freezeAtt     = std::make_unique<ButtonAttachment>   (s, nscm::params::midiFreeze, freezeButton);
    sideMuteAtt   = std::make_unique<ButtonAttachment>   (s, nscm::params::sideMute, sideMuteButton);
    multirateAtt  = std::make_unique<ButtonAttachment>   (s, nscm::params::multirate, multirateButton);

    currentTab = choiceIndex (s, nscm::params::uiTab);
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

void NSColourMapAudioProcessorEditor::configureRadio (juce::TextButton& button, const char* paramId, int valueIndex)
{
    button.setClickingTogglesState (false);
    button.onClick = [this, paramId, valueIndex] { setChoice (audioProcessor.getState(), paramId, valueIndex); syncRadios(); };
    addAndMakeVisible (button);
}

void NSColourMapAudioProcessorEditor::updateMainVisibility()
{
    const bool main = (currentTab == 0);
    juce::Component* mainComps[] = { &visualizer, &keyboard, &keyBox, &scaleBox, &freezeButton,
                                     &scaleShiftKnob, &scaleShiftLabel };
    for (auto* c : mainComps)
        c->setVisible (main);
    for (auto& b : gridModeButtons) b.setVisible (main);
    for (auto& b : characterButtons) b.setVisible (main);
    for (auto* s : { &colorKnob, &amountKnob, &formantKnob, &transientKnob, &mixKnob, &outputKnob })
        s->setVisible (main);
    for (auto* l : { &colorLabel, &amountLabel, &formantLabel, &transientLabel, &mixLabel, &outputLabel })
        l->setVisible (main);

    const bool adv = main && showAdvanced;
    for (auto* s : { &gammaKnob, &morphKnob, &gateKnob, &lowCutKnob, &highCutKnob })
        s->setVisible (adv);
    for (auto* l : { &gammaLabel, &morphLabel, &gateLabel, &lowCutLabel, &highCutLabel })
        l->setVisible (adv);
    sideMuteButton.setVisible (adv);
    multirateButton.setVisible (adv);
}

void NSColourMapAudioProcessorEditor::setCurrentTab (int index)
{
    currentTab = juce::jlimit (0, 1, index);
    setChoice (audioProcessor.getState(), nscm::params::uiTab, currentTab);
    mainTab.setToggleState  (currentTab == 0, juce::dontSendNotification);
    aboutTab.setToggleState (currentTab == 1, juce::dontSendNotification);
    updateMainVisibility();
    resized();
    repaint();
}

void NSColourMapAudioProcessorEditor::syncRadios()
{
    auto& s = audioProcessor.getState();
    const int mode = choiceIndex (s, nscm::params::mode);
    for (int i = 0; i < (int) gridModeButtons.size(); ++i)
        gridModeButtons[(size_t) i].setToggleState (i == mode, juce::dontSendNotification);
    const int ch = choiceIndex (s, nscm::params::character);
    for (int i = 0; i < (int) characterButtons.size(); ++i)
        characterButtons[(size_t) i].setToggleState (i == ch, juce::dontSendNotification);
}

void NSColourMapAudioProcessorEditor::timerCallback()
{
    syncRadios();
    repaint (midiIndicator.getBounds().expanded (8));

    const int q = choiceIndex (audioProcessor.getState(), nscm::params::quality);
    qualityButton.setButtonText (nscm::params::qualityNames()[q]);
    qualityButton.setToggleState (q == 1, juce::dontSendNotification);
    advancedButton.setToggleState (showAdvanced, juce::dontSendNotification);
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

    if (currentTab == 1)
    {
        auto area = getLocalBounds();
        area.removeFromTop (54);
        g.setColour (panel);
        g.fillRoundedRectangle (getLocalBounds().reduced (22).withTrimmedTop (32).toFloat(), 8.0f);
        area.reduce (36, 24);

        g.setFont (titleFont());
        g.setColour (text);
        g.drawText ("NSColourMap", area.removeFromTop (40), juce::Justification::centred);

        const auto badge = juce::Rectangle<float> ((float) getWidth() * 0.5f - 32.0f, (float) area.getY() + 1.0f, 64.0f, 22.0f);
        g.setColour (accent.withAlpha (0.15f));
        g.fillRoundedRectangle (badge, 5.0f);
        g.setColour (accent);
        g.setFont (sectionFont());
        g.drawText ("v0.5.0", badge.toNearestInt(), juce::Justification::centred);
        area.removeFromTop (34);

        g.setColour (panelLight.brighter (0.1f));
        g.fillRect (juce::Rectangle<float> ((float) getWidth() * 0.32f, (float) area.getY(), (float) getWidth() * 0.36f, 1.0f));
        area.removeFromTop (16);

        g.setFont (juce::Font (juce::FontOptions{}.withHeight (14.0f)));
        g.setColour (mutedText);
        g.drawText ("Pitch-grid colour mapping for Colour Bass", area.removeFromTop (22), juce::Justification::centred, true);
        g.setFont (juce::Font (juce::FontOptions{}.withHeight (13.0f)));
        g.setColour (text.withAlpha (0.65f));
        g.drawText ("PITCHMAP::COLORS x Chroma chimera  -  by nisesimadao", area.removeFromTop (22), juce::Justification::centred, true);
        area.removeFromTop (18);

        g.setFont (juce::Font (juce::FontOptions{}.withHeight (12.0f)));
        g.setColour (mutedText.withAlpha (0.75f));
        g.drawText ("Choose Key/Scale or send MIDI, then turn COLOR.", area.removeFromTop (20), juce::Justification::centred, true);
    }
}

void NSColourMapAudioProcessorEditor::resized()
{
    auto area   = getLocalBounds();
    auto header = area.removeFromTop (48).reduced (14, 6);
    logoLabel.setBounds (header.removeFromLeft (150));
    aboutTab.setBounds (header.removeFromRight (66));
    mainTab.setBounds (header.removeFromRight (62).reduced (4, 0));
    advancedButton.setBounds (header.removeFromRight (54).reduced (4, 4));
    qualityButton.setBounds (header.removeFromRight (74).reduced (4, 4));
    const auto led = header.removeFromRight (20);
    midiIndicator.setBounds (led.getCentreX() - 6, led.getCentreY() - 6, 12, 12);
    header.removeFromLeft (6);
    presetBox.setBounds (header.removeFromLeft (juce::jmin (190, header.getWidth())).reduced (0, 8));

    if (currentTab != 0)
        return;

    area.reduce (14, 12);

    // Key strip
    auto keyStrip = area.removeFromTop (30);
    {
        const int gmW = 64;
        for (auto& b : gridModeButtons)
            b.setBounds (keyStrip.removeFromLeft (gmW).reduced (2, 2));
        keyStrip.removeFromLeft (8);
        freezeButton.setBounds (keyStrip.removeFromRight (92).reduced (2, 2));
        keyStrip.removeFromRight (6);
        auto shiftCol = keyStrip.removeFromRight (54);
        scaleShiftLabel.setBounds (shiftCol.removeFromTop (12));
        scaleShiftKnob.setBounds (shiftCol);
        keyStrip.removeFromRight (8);
        scaleBox.setBounds (keyStrip.removeFromRight (juce::jmin (150, keyStrip.getWidth() / 2)).reduced (2, 2));
        keyBox.setBounds (keyStrip.removeFromRight (juce::jmin (70, keyStrip.getWidth())).reduced (2, 2));
    }
    area.removeFromTop (8);

    // Bottom-up: advanced (optional), character row, color section
    if (showAdvanced)
    {
        auto adv = area.removeFromBottom (92);
        const int n = 5;
        const int w = (adv.getWidth() - 150) / n;
        auto knobs = adv.removeFromLeft (w * n);
        juce::Slider* ks[] = { &gammaKnob, &morphKnob, &gateKnob, &lowCutKnob, &highCutKnob };
        juce::Label*  ls[] = { &gammaLabel, &morphLabel, &gateLabel, &lowCutLabel, &highCutLabel };
        for (int i = 0; i < n; ++i)
        {
            auto col = knobs.removeFromLeft (w).reduced (3, 0);
            ls[i]->setBounds (col.removeFromTop (15));
            ks[i]->setBounds (col);
        }
        adv.removeFromLeft (8);
        auto toggles = adv.reduced (2, 18);
        sideMuteButton.setBounds (toggles.removeFromTop (26));
        toggles.removeFromTop (4);
        multirateButton.setBounds (toggles.removeFromTop (26));
        area.removeFromBottom (6);
    }

    // Character row (Clean / Color / Hyper / Alien / Glitch) — below the colour section.
    auto charRow = area.removeFromBottom (28);
    {
        const int n = (int) characterButtons.size();
        const int w = charRow.getWidth() / n;
        for (int i = 0; i < n; ++i)
            characterButtons[(size_t) i].setBounds (
                charRow.removeFromLeft (i == n - 1 ? charRow.getWidth() : w).reduced (2, 2));
    }
    area.removeFromBottom (6);

    // Big COLOR + main knobs.
    auto colorSection = area.removeFromBottom (132);
    {
        auto big = colorSection.removeFromLeft (juce::jlimit (130, 200, colorSection.getWidth() / 3));
        colorLabel.setBounds (big.removeFromTop (16));
        colorKnob.setBounds (big);

        juce::Slider* ks[] = { &amountKnob, &formantKnob, &transientKnob, &mixKnob, &outputKnob };
        juce::Label*  ls[] = { &amountLabel, &formantLabel, &transientLabel, &mixLabel, &outputLabel };
        const int n = 5;
        const int w = colorSection.getWidth() / n;
        for (int i = 0; i < n; ++i)
        {
            auto col = colorSection.removeFromLeft (i == n - 1 ? colorSection.getWidth() : w).reduced (3, 6);
            ls[i]->setBounds (col.removeFromTop (15));
            ks[i]->setBounds (col);
        }
    }
    area.removeFromBottom (6);

    keyboard.setBounds (area.removeFromBottom (58));
    area.removeFromBottom (8);

    visualizer.setBounds (area);
}
