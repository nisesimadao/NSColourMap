#include "PluginEditor.h"

#include "Parameters.h"
#include "Presets.h"
#include "dsp/CharacterModes.h"
#include "dsp/ScaleNoteSet.h"

#include <cmath>

namespace
{
const juce::Colour background  { 0xff0b0e11 };
const juce::Colour panel       { 0xff1b2026 };
const juce::Colour panelLight  { 0xff2b333d };
const juce::Colour glassEdge   { 0x3effffff };
const juce::Colour glassShadow { 0x7a000000 };
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

void drawWallpaperBackdrop (juce::Graphics& g, juce::Rectangle<float> r)
{
    juce::ColourGradient base (juce::Colour { 0xff121820u }, r.getX(), r.getY(),
                               juce::Colour { 0xff050609u }, r.getX(), r.getBottom(), false);
    base.addColour (0.36, juce::Colour { 0xff0b1118u });
    base.addColour (0.72, juce::Colour { 0xff07080du });
    g.setGradientFill (base);
    g.fillRect (r);

    auto radial = [&] (juce::Colour c, float x, float y, float radius, float alpha)
    {
        juce::ColourGradient grad (c.withAlpha (alpha), x, y,
                                   c.withAlpha (0.0f), x + radius, y - radius * 0.25f, true);
        g.setGradientFill (grad);
        g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre ({ x, y }));
    };

    radial (juce::Colour { 0xff1d7c8du }, r.getX() + r.getWidth() * 0.18f, r.getY() + r.getHeight() * 0.18f, r.getWidth() * 0.46f, 0.34f);
    radial (juce::Colour { 0xff735140u }, r.getX() + r.getWidth() * 0.86f, r.getY() + r.getHeight() * 0.29f, r.getWidth() * 0.36f, 0.20f);
    radial (juce::Colour { 0xff6747a2u }, r.getX() + r.getWidth() * 0.58f, r.getY() + r.getHeight() * 0.82f, r.getWidth() * 0.50f, 0.16f);

    auto fillRibbon = [&] (float yBase, float ampA, float ampB, juce::Colour left, juce::Colour right, float alpha)
    {
        juce::Path p;
        p.startNewSubPath (r.getX() - 80.0f, yBase);
        for (int i = 0; i <= 14; ++i)
        {
            const float t = (float) i / 14.0f;
            const float x = r.getX() + r.getWidth() * t;
            const float y = yBase + std::sin ((t + 0.08f) * juce::MathConstants<float>::twoPi * 0.92f) * ampA
                                  + std::sin ((t + 0.27f) * juce::MathConstants<float>::twoPi * 2.10f) * ampB;
            p.lineTo (x, y);
        }
        p.lineTo (r.getRight() + 80.0f, r.getBottom() + 70.0f);
        p.lineTo (r.getX() - 80.0f, r.getBottom() + 70.0f);
        p.closeSubPath();

        juce::ColourGradient grad (left.withAlpha (alpha), r.getX(), yBase,
                                   right.withAlpha (alpha * 0.42f), r.getRight(), r.getBottom(), false);
        g.setGradientFill (grad);
        g.fillPath (p);
    };

    fillRibbon (r.getY() + r.getHeight() * 0.27f, 56.0f, 18.0f,
                juce::Colour { 0xff1a6d78u }, juce::Colour { 0xff2b1636u }, 0.18f);
    fillRibbon (r.getY() + r.getHeight() * 0.47f, 42.0f, 14.0f,
                juce::Colour { 0xff0a3038u }, juce::Colour { 0xff7c4732u }, 0.11f);

    juce::Path ribbon;
    const float top = r.getY() + r.getHeight() * 0.20f;
    ribbon.startNewSubPath (r.getX() - 40.0f, top);
    for (int i = 0; i <= 8; ++i)
    {
        const float t = (float) i / 8.0f;
        const float x = r.getX() + r.getWidth() * t;
        const float y = top + std::sin (t * juce::MathConstants<float>::twoPi * 1.35f) * 34.0f
                            + std::sin (t * juce::MathConstants<float>::twoPi * 3.1f) * 10.0f;
        ribbon.lineTo (x, y);
    }
    ribbon.lineTo (r.getRight() + 40.0f, r.getBottom() + 40.0f);
    ribbon.lineTo (r.getX() - 40.0f, r.getBottom() + 40.0f);
    ribbon.closeSubPath();

    juce::ColourGradient veil (juce::Colour { 0x225ec8d8u }, r.getX(), top,
                               juce::Colour { 0x051a1026u }, r.getRight(), r.getBottom(), false);
    g.setGradientFill (veil);
    g.fillPath (ribbon);

    g.setColour (juce::Colours::white.withAlpha (0.022f));
    for (int i = 0; i < 10; ++i)
    {
        const float y = r.getY() + 64.0f + (float) i * 92.0f;
        g.drawHorizontalLine (juce::roundToInt (y), r.getX(), r.getRight());
    }

    g.setColour (juce::Colours::black.withAlpha (0.22f));
    g.fillRect (r);
}

void drawLiquidPanel (juce::Graphics& g, juce::Rectangle<float> r, float radius = 8.0f)
{
    g.setColour (glassShadow.withAlpha (0.34f));
    g.fillRoundedRectangle (r.translated (0.0f, 9.0f).expanded (-2.0f, 0.0f), radius + 2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.18f));
    g.fillRoundedRectangle (r.expanded (2.0f).translated (0.0f, 2.0f), radius + 2.0f);

    juce::ColourGradient material (juce::Colour { 0x2a27313au }, r.getX(), r.getY(),
                                   juce::Colour { 0x26101419u }, r.getX(), r.getBottom(), false);
    material.addColour (0.22, juce::Colour { 0x203a4650u });
    material.addColour (0.58, juce::Colour { 0x16161d23u });
    material.addColour (1.0,  juce::Colour { 0x3005080bu });
    g.setGradientFill (material);
    g.fillRoundedRectangle (r, radius);

    const auto inner = r.reduced (1.0f);
    juce::ColourGradient depth (juce::Colours::white.withAlpha (0.040f), inner.getX(), inner.getY(),
                                juce::Colours::black.withAlpha (0.14f), inner.getX(), inner.getBottom(), false);
    g.setGradientFill (depth);
    g.fillRoundedRectangle (inner, radius - 1.0f);

    g.setColour (juce::Colours::white.withAlpha (0.085f));
    g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);

    g.setColour (juce::Colours::white.withAlpha (0.060f));
    g.drawLine (r.getX() + radius, r.getY() + 1.0f, r.getRight() - radius, r.getY() + 1.0f, 1.0f);

    g.setColour (juce::Colours::black.withAlpha (0.22f));
    g.drawRoundedRectangle (r.reduced (1.5f), radius - 1.0f, 1.0f);
}
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
                                                   const juce::Colour&, bool highlighted, bool down)
{
    const auto bounds = button.getLocalBounds().toFloat();

    // Eased select/hover state (animated) when available.
    float sel = button.getToggleState() ? 1.0f : 0.0f;
    float hov = highlighted ? 1.0f : 0.0f;
    if (auto* ab = dynamic_cast<AnimatedButton*> (&button)) { sel = ab->sel; hov = ab->hover; }

    if (hov > 0.001f)
    {
        g.setColour (juce::Colours::white.withAlpha ((down ? 0.09f : 0.05f) * hov));
        g.fillRoundedRectangle (bounds.reduced (1.0f, 2.0f), 4.0f);
    }
    if (sel > 0.001f)
    {
        g.setColour (accent.withAlpha (0.14f * sel));
        g.fillRoundedRectangle (bounds.reduced (1.0f, 2.0f), 4.0f);

        // Underline grows out from the centre as the button becomes selected.
        const float fullW = bounds.getWidth() - 14.0f;
        const float w = fullW * sel;
        g.setColour (accent.withAlpha (sel));
        g.fillRoundedRectangle (juce::Rectangle<float> (bounds.getCentreX() - w * 0.5f,
                                                        bounds.getBottom() - 3.0f, w, 2.5f), 1.2f);
    }
}

void NSColourMapLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                          int, int, int, int, juce::ComboBox&)
{
    const auto bounds = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);
    juce::ColourGradient fill (juce::Colour { 0x42394752u }, bounds.getX(), bounds.getY(),
                               juce::Colour { 0x2b161b21u }, bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill (fill);
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (juce::Colours::white.withAlpha (0.14f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.18f));
    g.drawRoundedRectangle (bounds.reduced (1.5f), 3.0f, 1.0f);

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
                                              float sliderPos, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
    // Eased "push in" amount while the knob is held (set by the editor timer).
    const float press = (float) slider.getProperties().getWithDefault ("press", 0.0);

    const auto rawBounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (4.0f);
    const auto diameter0 = juce::jmin (rawBounds.getWidth(), rawBounds.getHeight());
    const auto diameter  = diameter0 * (1.0f - 0.06f * press);   // shrink when pressed
    const auto bounds    = juce::Rectangle<float> (diameter, diameter).withCentre (rawBounds.getCentre());
    const auto radius    = bounds.getWidth() * 0.5f;
    const auto centre    = bounds.getCentre();
    const auto angle     = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const float arcThick = juce::jmax (3.0f, radius * 0.11f);

    // Body — darkens slightly when pushed in.
    const float dim = 0.18f * press;
    g.setGradientFill (juce::ColourGradient (juce::Colour { 0xff3d444du }.darker (dim), centre.x, bounds.getY(),
                                             juce::Colour { 0xff171a1du }.darker (dim), centre.x, bounds.getBottom(), false));
    g.fillEllipse (bounds);
    g.setColour (juce::Colour { 0xff090a0cu });
    g.drawEllipse (bounds, 1.5f);

    // Inner shadow ring to read as "depressed".
    if (press > 0.001f)
    {
        g.setColour (juce::Colours::black.withAlpha (0.30f * press));
        g.drawEllipse (bounds.reduced (1.5f), 2.0f);
    }

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

    float sel = button.getToggleState() ? 1.0f : 0.0f;
    float hov = highlighted ? 1.0f : 0.0f;
    if (auto* at = dynamic_cast<AnimatedToggle*> (&button)) { sel = at->sel; hov = at->hover; }

    juce::ColourGradient base (juce::Colour { 0x3a34404au }.brighter (0.08f * hov), bounds.getX(), bounds.getY(),
                               juce::Colour { 0x24101418u }, bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill (base);
    g.fillRoundedRectangle (bounds, 4.0f);
    if (sel > 0.001f)
    {
        g.setColour (accent.withAlpha (0.14f * sel));
        g.fillRoundedRectangle (bounds.reduced (1.0f), 3.5f);
    }
    g.setColour (juce::Colours::white.withAlpha (0.11f + 0.16f * sel));
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f + 0.8f * sel);

    // Tick dot grows + brightens as it turns on.
    const float maxTick = juce::jmin (bounds.getHeight() * 0.42f, 9.0f);
    const float tickSize = maxTick * (0.7f + 0.3f * sel);
    const float tickCx   = bounds.getX() + 7.0f + maxTick * 0.5f;
    const float tickCy   = bounds.getCentreY();
    g.setColour (mutedText.overlaidWith (accent.withAlpha (sel)));
    g.fillEllipse (tickCx - tickSize * 0.5f, tickCy - tickSize * 0.5f, tickSize, tickSize);

    g.setFont (sectionFont());
    g.setColour (mutedText.overlaidWith (text.withAlpha (sel)));
    auto textArea = bounds.toNearestInt();
    textArea.removeFromLeft ((int) (7.0f + maxTick) + 5);
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
    juce::ColourGradient glass (juce::Colour { 0x382a323au }, bounds.getX(), bounds.getY(),
                                juce::Colour { 0x24101418u }, bounds.getX(), bounds.getBottom(), false);
    glass.addColour (0.45, juce::Colour { 0x20202a32u });
    g.setGradientFill (glass);
    g.fillRoundedRectangle (bounds, 8.0f);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawRoundedRectangle (bounds, 8.0f, 1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.20f));
    g.drawRoundedRectangle (bounds.reduced (1.5f), 7.0f, 1.0f);

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

    // EQ-style spectrum analyzer (output), drawn behind the pitch lanes.
    {
        const int n = processor.getSpectrumBinCount();
        constexpr float aLo = 30.0f, aHi = 18000.0f;
        const float lr = std::log (aHi / aLo);
        juce::Path curve, fill;
        bool started = false;
        for (int i = 0; i < n; ++i)
        {
            const float f = aLo * std::exp (lr * (float) i / (float) (n - 1));
            const float xx = freqToX (f, plot);
            const float v  = processor.getSpectrumBin (i);
            const float yy = plot.getBottom() - v * plot.getHeight() * 0.92f;
            if (! started) { curve.startNewSubPath (xx, yy); fill.startNewSubPath (xx, plot.getBottom()); fill.lineTo (xx, yy); started = true; }
            else           { curve.lineTo (xx, yy); fill.lineTo (xx, yy); }
        }
        fill.lineTo (plot.getRight(), plot.getBottom());
        fill.closeSubPath();
        g.setColour (accent.withAlpha (0.10f));
        g.fillPath (fill);
        g.setColour (accent.withAlpha (0.45f));
        g.strokePath (curve, juce::PathStrokeType (1.4f));
    }

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

KeyboardView::KeyboardView (NSColourMapAudioProcessor& p) : processor (p)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    startTimerHz (20);
}
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

int KeyboardView::pitchClassAt (juce::Point<float> p) const
{
    const auto bounds = getLocalBounds().toFloat().reduced (1.0f);
    const int whiteCount = 14;
    const float kbX = bounds.getX() + 4.0f, kbY = bounds.getY() + 3.0f;
    const float kbW = bounds.getWidth() - 8.0f, kbH = bounds.getHeight() - 6.0f;
    const float whiteW = kbW / (float) whiteCount;
    static constexpr int whitePcs[7]   = { 0, 2, 4, 5, 7, 9, 11 };
    static constexpr int blackAfter[7] = { 1, 1, 0, 1, 1, 1, 0 };
    const float blackW = whiteW * 0.62f, blackH = kbH * 0.6f;

    // Black keys are on top — test them first.
    for (int i = 0; i < whiteCount; ++i)
    {
        if (! blackAfter[i % 7]) continue;
        const float bx = kbX + (i + 1) * whiteW - blackW * 0.5f;
        if (juce::Rectangle<float> (bx, kbY, blackW, blackH).contains (p))
            return (whitePcs[i % 7] + 1) % 12;
    }
    if (p.x < kbX || p.x > kbX + kbW) return -1;
    const int idx = juce::jlimit (0, whiteCount - 1, (int) ((p.x - kbX) / whiteW));
    return whitePcs[idx % 7];
}

void KeyboardView::mouseDown (const juce::MouseEvent& e)
{
    const int pc = pitchClassAt (e.position);
    if (pc < 0) return;

    auto& s = processor.getState();
    if (choiceIndex (s, nscm::params::mode) == 3) // UI grid: toggle the clicked note
    {
        const juce::uint32 m = processor.getCustomMask() ^ (1u << pc);
        processor.setCustomMask (m);
    }
    else // otherwise set the root (Key)
    {
        setChoice (s, nscm::params::key, pc);
    }
    repaint();
}

void KeyboardView::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced (1.0f);
    juce::ColourGradient bed (juce::Colour { 0x322b333bu }, bounds.getX(), bounds.getY(),
                              juce::Colour { 0x27080b0eu }, bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill (bed);
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (juce::Colours::white.withAlpha (0.055f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);

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

    qualityLabel.setText ("Quality", juce::dontSendNotification);
    qualityLabel.setFont (juce::Font (juce::FontOptions (9.0f)));
    qualityLabel.setColour (juce::Label::textColourId, mutedText);
    qualityLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (qualityLabel);

    qualitySlider.setSliderStyle (juce::Slider::LinearHorizontal);
    qualitySlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 54, 18);
    qualitySlider.setColour (juce::Slider::textBoxTextColourId, text);
    qualitySlider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour { 0x00000000u });
    qualitySlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colour { 0x00000000u });
    qualitySlider.setTooltip (juce::String::fromUTF8 ("Quality: 0 Latency(遅延ゼロ・プレビュー向き) → Low/Mid/High(STFTで高音質・レイテンシ増)。書き出しはどれでもPDC補正で同期"));
    addAndMakeVisible (qualitySlider);

    advancedButton.setClickingTogglesState (false);
    advancedButton.setTooltip (juce::String::fromUTF8 ("詳細パラメータ (Gamma / Morph / Gate / Low・High Cut など) の表示切替"));
    advancedButton.onClick = [this] { showAdvanced = ! showAdvanced; advancedButton.setToggleState (showAdvanced, juce::dontSendNotification); updateMainVisibility(); resized(); repaint(); };
    addAndMakeVisible (advancedButton);

    styleButton.setClickingTogglesState (false);
    styleButton.setTooltip (juce::String::fromUTF8 ("UIスタイル切替: Clean(分かりやすい) / Classic(従来)"));
    styleButton.onClick = [this] { setUiStyle (uiStyle == 0 ? 1 : 0); };
    addAndMakeVisible (styleButton);

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
    configureKnob (melodyKnob, melodyLabel, "Melody");
    configureKnob (formantKnob, formantLabel, "Formant");
    configureKnob (transientKnob, transientLabel, "Transient");
    configureKnob (mixKnob, mixLabel, "Mix");
    configureKnob (outputKnob, outputLabel, "Output");

    configureKnob (gammaKnob, gammaLabel, "Gamma");
    configureKnob (morphKnob, morphLabel, "Morph");
    configureKnob (gateKnob, gateLabel, "Gate");
    configureKnob (airKnob, airLabel, "Air");
    configureKnob (lowCutKnob, lowCutLabel, "Low Cut");
    configureKnob (highCutKnob, highCutLabel, "High Cut");

    // Japanese tooltips (分かりやすさ)
    using JS = juce::String;
    colorKnob.setTooltip     (JS::fromUTF8 ("COLOR: 色の量。0-100%でドライ→キーに染める、100-200%で共鳴と煌びやかなテイルを追加。まずこれを回す"));
    amountKnob.setTooltip     (JS::fromUTF8 ("Amount: グリッド(キー)へ寄せる強さ"));
    melodyKnob.setTooltip     (JS::fromUTF8 ("Melody: 今のシャリシャリ感を残しつつ、上げるほど入力に近いグリッド音を前に出してメロディックにする"));
    formantKnob.setTooltip    (JS::fromUTF8 ("Formant: 母音/サイズ感（−で太く低く、＋で細く高く）"));
    transientKnob.setTooltip  (JS::fromUTF8 ("Transient: アタックをドライのまま通す量"));
    mixKnob.setTooltip        (JS::fromUTF8 ("Mix: 原音と処理音のバランス"));
    outputKnob.setTooltip     (JS::fromUTF8 ("Output: 出力音量"));
    gammaKnob.setTooltip      (JS::fromUTF8 ("Gamma: フォルマントの山谷を誇張し母音をゆっくり揺らす（有機的な動き）"));
    morphKnob.setTooltip      (JS::fromUTF8 ("Morph: 原音の輪郭/ダイナミクスを処理音に転写（アタック保持・テイル抑制）"));
    gateKnob.setTooltip       (JS::fromUTF8 ("Gate: 共鳴のテイルを締める"));
    airKnob.setTooltip        (JS::fromUTF8 ("Air: なめらかな高域の粒立ち（シャリシャリ）。刺さらずに空気感・煌めきを足す"));
    lowCutKnob.setTooltip     (JS::fromUTF8 ("Low Cut: この周波数以下は無加工で保護（サブを濁らせない）"));
    highCutKnob.setTooltip    (JS::fromUTF8 ("High Cut: この周波数以上は処理しない"));
    scaleShiftKnob.setTooltip (JS::fromUTF8 ("Scale Shift: グリッド全体を半音単位で移動（オートメーション向き）"));
    keyBox.setTooltip         (JS::fromUTF8 ("Key: 曲のキー（ルート音）"));
    scaleBox.setTooltip       (JS::fromUTF8 ("Scale: スケール。Colour Bassは Minor Pentatonic / Pentatonic Blues が定番"));
    freezeButton.setTooltip   (JS::fromUTF8 ("Freeze: Offでノートを離すと止まる / Onで最後のコードを保持"));
    gridModeButtons[0].setTooltip (JS::fromUTF8 ("Scale: Key/Scaleでターゲット音を決める（MIDI不要）"));
    gridModeButtons[1].setTooltip (JS::fromUTF8 ("MIDI: 送ったMIDIコードでターゲット音を決める"));
    gridModeButtons[2].setTooltip (JS::fromUTF8 ("Hybrid: スケールを土台にMIDIで弾いた音を強調"));
    gridModeButtons[3].setTooltip (JS::fromUTF8 ("UI: 下の鍵盤をクリックして音を選ぶ"));
    gridModeButtons[4].setTooltip (JS::fromUTF8 ("Audio: 入力音声から音名を検出してターゲットにする（MIDI不要）"));

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
    melodyAtt     = std::make_unique<SliderAttachment> (s, nscm::params::melody,     melodyKnob);
    formantAtt    = std::make_unique<SliderAttachment> (s, nscm::params::formant,    formantKnob);
    transientAtt  = std::make_unique<SliderAttachment> (s, nscm::params::transient,  transientKnob);
    mixAtt        = std::make_unique<SliderAttachment> (s, nscm::params::mix,        mixKnob);
    outputAtt     = std::make_unique<SliderAttachment> (s, nscm::params::output,     outputKnob);
    gammaAtt      = std::make_unique<SliderAttachment> (s, nscm::params::gamma,      gammaKnob);
    morphAtt      = std::make_unique<SliderAttachment> (s, nscm::params::morph,      morphKnob);
    gateAtt       = std::make_unique<SliderAttachment> (s, nscm::params::gate,       gateKnob);
    airAtt        = std::make_unique<SliderAttachment> (s, nscm::params::air,        airKnob);
    lowCutAtt     = std::make_unique<SliderAttachment> (s, nscm::params::lowCut,     lowCutKnob);
    highCutAtt    = std::make_unique<SliderAttachment> (s, nscm::params::highCut,    highCutKnob);
    scaleShiftAtt = std::make_unique<SliderAttachment> (s, nscm::params::scaleShift, scaleShiftKnob);
    qualityAtt    = std::make_unique<SliderAttachment> (s, nscm::params::quality,    qualitySlider);
    keyAtt        = std::make_unique<ComboBoxAttachment> (s, nscm::params::key,      keyBox);
    scaleAtt      = std::make_unique<ComboBoxAttachment> (s, nscm::params::scale,    scaleBox);
    freezeAtt     = std::make_unique<ButtonAttachment>   (s, nscm::params::midiFreeze, freezeButton);
    sideMuteAtt   = std::make_unique<ButtonAttachment>   (s, nscm::params::sideMute, sideMuteButton);
    multirateAtt  = std::make_unique<ButtonAttachment>   (s, nscm::params::multirate, multirateButton);

    uiStyle = juce::jlimit (0, 1, (int) s.state.getProperty ("uiStyle", 0));
    styleButton.setButtonText (uiStyle == 0 ? "Clean" : "Classic");

    currentTab = choiceIndex (s, nscm::params::uiTab);
    setCurrentTab (currentTab);
    startTimerHz (30);
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
    for (auto* s : { &colorKnob, &amountKnob, &melodyKnob, &formantKnob, &transientKnob, &mixKnob, &outputKnob })
        s->setVisible (main);
    for (auto* l : { &colorLabel, &amountLabel, &melodyLabel, &formantLabel, &transientLabel, &mixLabel, &outputLabel })
        l->setVisible (main);

    const bool adv = main && showAdvanced;
    for (auto* s : { &gammaKnob, &morphKnob, &gateKnob, &airKnob, &lowCutKnob, &highCutKnob })
        s->setVisible (adv);
    for (auto* l : { &gammaLabel, &morphLabel, &gateLabel, &airLabel, &lowCutLabel, &highCutLabel })
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

    // Eased button select/hover transitions.
    advancedButton.setToggleState (showAdvanced, juce::dontSendNotification);
    auto animBtn = [] (auto& b) { if (stepButtonAnim (b)) b.repaint(); };
    for (auto& b : gridModeButtons)  animBtn (b);
    for (auto& b : characterButtons) animBtn (b);
    animBtn (mainTab); animBtn (aboutTab); animBtn (styleButton); animBtn (advancedButton);
    animBtn (freezeButton); animBtn (sideMuteButton); animBtn (multirateButton);

    // Eased "push in" on knobs while held/dragged.
    juce::Slider* knobs[] = { &colorKnob, &amountKnob, &melodyKnob, &formantKnob, &transientKnob, &mixKnob, &outputKnob,
                              &gammaKnob, &morphKnob, &gateKnob, &airKnob, &lowCutKnob, &highCutKnob, &scaleShiftKnob };
    for (auto* k : knobs)
    {
        const float cur = (float) k->getProperties().getWithDefault ("press", 0.0);
        const float tgt = k->isMouseButtonDown() ? 1.0f : 0.0f;
        const float nv  = cur + (tgt > cur ? 0.55f : 0.22f) * (tgt - cur);
        if (std::abs (nv - cur) > 0.004f)      { k->getProperties().set ("press", nv);  k->repaint(); }
        else if (std::abs (tgt - cur) > 1.0e-4f) { k->getProperties().set ("press", tgt); k->repaint(); }
    }

    // Ease the COLOR glow toward the live colour energy (fast attack, slow release).
    const float target = juce::jlimit (0.0f, 1.0f,
                         (audioProcessor.getColoredEnergy() * 6.0f + audioProcessor.getTunedEnergy() * 3.0f));
    const float rate = target > glowLevel ? 0.5f : 0.12f;
    const float newGlow = glowLevel + rate * (target - glowLevel);
    if (std::abs (newGlow - glowLevel) > 0.002f)
    {
        glowLevel = newGlow;
        if (currentTab == 0 && uiStyle == 0)
            repaint (colorKnob.getBounds().expanded (26));
    }
}

void NSColourMapAudioProcessorEditor::paint (juce::Graphics& g)
{
    drawWallpaperBackdrop (g, getLocalBounds().toFloat());

    auto bounds = getLocalBounds();
    const auto topBar = bounds.removeFromTop (48).toFloat();
    juce::ColourGradient topGlass (juce::Colour { 0x44252d35u }, 0.0f, topBar.getY(),
                                   juce::Colour { 0x35101418u }, 0.0f, topBar.getBottom(), false);
    topGlass.addColour (0.18, juce::Colour { 0x302f3a44u });
    g.setGradientFill (topGlass);
    g.fillRect (topBar);
    g.setColour (juce::Colours::white.withAlpha (0.035f));
    g.drawHorizontalLine (0, 0.0f, (float) getWidth());
    g.setColour (juce::Colours::black.withAlpha (0.52f));
    g.drawHorizontalLine (48, 0.0f, (float) getWidth());
    g.setColour (juce::Colours::white.withAlpha (0.055f));
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

    // Clean-mode section panels + titles
    if (currentTab == 0 && uiStyle == 0)
    {
        auto drawPanel = [&] (juce::Rectangle<int> r)
        {
            if (r.isEmpty()) return;
            drawLiquidPanel (g, r.toFloat(), 8.0f);
        };
        auto drawTitle = [&] (juce::Rectangle<int> r, const char* t, juce::Justification j)
        {
            if (r.isEmpty()) return;
            g.setColour (accent.withAlpha (0.85f));
            g.setFont (juce::Font (juce::FontOptions (10.0f).withStyle ("Bold")));
            g.drawText (t, r, j);
        };
        drawPanel (rSourcePanel);
        drawPanel (rEnginePanel);

        // Glow behind the hero COLOR knob — driven by the actual colour energy
        // being generated (meaningful, eased), not a decorative pulse.
        if (! colorKnob.getBounds().isEmpty() && glowLevel > 0.01f)
        {
            const auto kb = colorKnob.getBounds().toFloat();
            const float r = kb.getWidth() * (0.52f + 0.10f * glowLevel);
            const auto c = kb.getCentre();
            juce::ColourGradient grad (accent.withAlpha (0.30f * glowLevel), c.x, c.y,
                                       accent.withAlpha (0.0f), c.x, c.y - r, true);
            g.setGradientFill (grad);
            g.fillEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c));
        }

        drawTitle (rTitleSource, "SOURCE",    juce::Justification::centredLeft);
        drawTitle (rTitleChar,   "CHARACTER", juce::Justification::centredLeft);
        drawTitle (rTitleColor,  "COLOR",     juce::Justification::centred);
        drawTitle (rTitleTone,   "TONE",      juce::Justification::centredLeft);
    }

    if (currentTab == 1)
    {
        auto area = getLocalBounds();
        area.removeFromTop (54);
        const auto aboutPanel = getLocalBounds().reduced (22).withTrimmedTop (32).toFloat();
        drawLiquidPanel (g, aboutPanel, 8.0f);
        area.reduce (36, 24);

        g.setFont (titleFont());
        g.setColour (text);
        g.drawText ("NSColourMap", area.removeFromTop (40), juce::Justification::centred);

        const auto badge = juce::Rectangle<float> ((float) getWidth() * 0.5f - 32.0f, (float) area.getY() + 1.0f, 64.0f, 22.0f);
        g.setColour (accent.withAlpha (0.15f));
        g.fillRoundedRectangle (badge, 5.0f);
        g.setColour (accent);
        g.setFont (sectionFont());
        g.drawText ("v0.8.16", badge.toNearestInt(), juce::Justification::centred);
        area.removeFromTop (34);

        g.setColour (panelLight.brighter (0.1f));
        g.fillRect (juce::Rectangle<float> ((float) getWidth() * 0.32f, (float) area.getY(), (float) getWidth() * 0.36f, 1.0f));
        area.removeFromTop (16);

        g.setFont (juce::Font (juce::FontOptions{}.withHeight (14.0f)));
        g.setColour (mutedText);
        g.drawText ("Pitch-grid colour mapping for Colour Bass", area.removeFromTop (22), juce::Justification::centred, true);
        g.setFont (juce::Font (juce::FontOptions{}.withHeight (13.0f)));
        g.setColour (text.withAlpha (0.65f));
        g.drawText ("Pitch-grid mapping x fast colour workflow  -  by nisesimadao", area.removeFromTop (22), juce::Justification::centred, true);
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
    logoLabel.setBounds (header.removeFromLeft (138));
    aboutTab.setBounds (header.removeFromRight (56));
    mainTab.setBounds (header.removeFromRight (52).reduced (3, 0));
    styleButton.setBounds (header.removeFromRight (62).reduced (3, 4));
    advancedButton.setBounds (header.removeFromRight (46).reduced (3, 4));
    {
        auto qz = header.removeFromRight (132).reduced (3, 4);
        qualityLabel.setBounds (qz.removeFromLeft (38));
        qualitySlider.setBounds (qz);
    }
    const auto led = header.removeFromRight (18);
    midiIndicator.setBounds (led.getCentreX() - 6, led.getCentreY() - 6, 12, 12);
    header.removeFromLeft (6);
    presetBox.setBounds (header.removeFromLeft (juce::jmin (180, header.getWidth())).reduced (0, 8));

    if (currentTab != 0)
        return;

    area.reduce (14, 12);
    if (uiStyle == 0) layoutClean (area);
    else              layoutClassic (area);
}

void NSColourMapAudioProcessorEditor::layoutClassic (juce::Rectangle<int> area)
{
    rSourcePanel = rEnginePanel = {}; // not used in classic

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
        const int n = 6;
        const int w = (adv.getWidth() - 150) / n;
        auto knobs = adv.removeFromLeft (w * n);
        juce::Slider* ks[] = { &gammaKnob, &morphKnob, &gateKnob, &airKnob, &lowCutKnob, &highCutKnob };
        juce::Label*  ls[] = { &gammaLabel, &morphLabel, &gateLabel, &airLabel, &lowCutLabel, &highCutLabel };
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

    // Character row (Clean / Color / Hyper / Map / Glitch) — below the colour section.
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

        juce::Slider* ks[] = { &amountKnob, &melodyKnob, &formantKnob, &transientKnob, &mixKnob, &outputKnob };
        juce::Label*  ls[] = { &amountLabel, &melodyLabel, &formantLabel, &transientLabel, &mixLabel, &outputLabel };
        const int n = 6;
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

void NSColourMapAudioProcessorEditor::layoutClean (juce::Rectangle<int> area)
{
    // ── SOURCE panel (grid mode + key/scale/shift/freeze) ─────────────────────
    rSourcePanel = area.removeFromTop (58);
    {
        auto inner = rSourcePanel.reduced (12, 8);
        rTitleSource = inner.removeFromTop (14);
        inner.removeFromTop (2);
        const int gmW = juce::jmin (62, inner.getWidth() / 7);
        for (auto& b : gridModeButtons)
            b.setBounds (inner.removeFromLeft (gmW).reduced (2, 2));
        inner.removeFromLeft (10);
        freezeButton.setBounds (inner.removeFromRight (90).reduced (2, 3));
        inner.removeFromRight (8);
        auto shiftCol = inner.removeFromRight (50);
        scaleShiftLabel.setBounds (shiftCol.removeFromTop (11));
        scaleShiftKnob.setBounds (shiftCol);
        inner.removeFromRight (8);
        scaleBox.setBounds (inner.removeFromRight (juce::jmin (150, inner.getWidth() * 2 / 3)).reduced (2, 3));
        keyBox.setBounds (inner.removeFromRight (juce::jmin (64, inner.getWidth())).reduced (2, 3));
    }
    area.removeFromTop (10);

    // ── Advanced drawer (optional, bottom) ────────────────────────────────────
    if (showAdvanced)
    {
        auto adv = area.removeFromBottom (92);
        const int n = 6;
        const int w = (adv.getWidth() - 150) / n;
        auto knobs = adv.removeFromLeft (w * n);
        juce::Slider* ks[] = { &gammaKnob, &morphKnob, &gateKnob, &airKnob, &lowCutKnob, &highCutKnob };
        juce::Label*  ls[] = { &gammaLabel, &morphLabel, &gateLabel, &airLabel, &lowCutLabel, &highCutLabel };
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
        area.removeFromBottom (8);
    }

    // ── Engine zone: CHARACTER | COLOR (hero) | TONE ──────────────────────────
    rEnginePanel = area.removeFromBottom (188);
    {
        auto inner = rEnginePanel.reduced (12, 10);

        // Left: CHARACTER (vertical list)
        auto charCol = inner.removeFromLeft (140);
        rTitleChar = charCol.removeFromTop (15);
        charCol.removeFromTop (2);
        const int bh = juce::jmin (28, charCol.getHeight() / (int) characterButtons.size());
        for (auto& b : characterButtons)
            b.setBounds (charCol.removeFromTop (bh).reduced (1, 2));

        inner.removeFromLeft (12);

        // Center: hero COLOR
        auto colorCol = inner.removeFromLeft (juce::jlimit (150, 200, inner.getWidth() * 2 / 5));
        rTitleColor = colorCol.removeFromTop (15);
        colorKnob.setBounds (colorCol.reduced (4, 2));
        colorLabel.setBounds (0, 0, 0, 0); // title drawn by panel instead

        inner.removeFromLeft (12);

        // Right: TONE — Amount/Melody/Formant/Transient (row 1), Mix/Output (row 2)
        rTitleTone = inner.removeFromTop (15);
        inner.removeFromTop (2);
        auto row1 = inner.removeFromTop (inner.getHeight() / 2);
        auto row2 = inner;
        juce::Slider* r1k[] = { &amountKnob, &melodyKnob, &formantKnob, &transientKnob };
        juce::Label*  r1l[] = { &amountLabel, &melodyLabel, &formantLabel, &transientLabel };
        juce::Slider* r2k[] = { &mixKnob, &outputKnob };
        juce::Label*  r2l[] = { &mixLabel, &outputLabel };
        const int w1 = row1.getWidth() / 4;
        for (int i = 0; i < 4; ++i)
        {
            auto col = row1.removeFromLeft (i == 3 ? row1.getWidth() : w1).reduced (3, 1);
            r1l[i]->setBounds (col.removeFromTop (13));
            r1k[i]->setBounds (col);
        }
        const int w2 = row2.getWidth() / 3;
        for (int i = 0; i < 2; ++i)
        {
            auto col = row2.removeFromLeft (w2).reduced (3, 1);
            r2l[i]->setBounds (col.removeFromTop (13));
            r2k[i]->setBounds (col);
        }
    }
    area.removeFromBottom (10);

    // ── Keyboard + Visualizer ─────────────────────────────────────────────────
    keyboard.setBounds (area.removeFromBottom (56));
    area.removeFromBottom (8);
    visualizer.setBounds (area);
}

void NSColourMapAudioProcessorEditor::setUiStyle (int style)
{
    uiStyle = juce::jlimit (0, 1, style);
    audioProcessor.getState().state.setProperty ("uiStyle", uiStyle, nullptr);
    styleButton.setButtonText (uiStyle == 0 ? "Clean" : "Classic");
    resized();
    repaint();
}
