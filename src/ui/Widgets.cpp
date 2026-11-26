#include "Widgets.h"
#include "LookAndFeel.h"
#include "../dsp/Oscillator.h"
#include "../dsp/Lfo.h"
#include "../Parameters.h"

using namespace juce;

//==============================================================================
void HelpDisplay::setPreset (const String& bank, const String& name)
{
    if (bank != bankText || name != nameText)
    {
        bankText = bank;
        nameText = name;
        repaint();
    }
}

void HelpDisplay::setHelpText (const String& text)
{
    if (text != helpText)
    {
        helpText = text;
        repaint();
    }
}

void HelpDisplay::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (1.0f);
    AcidLookAndFeel::drawLcd (g, b);
    const auto& t = Theme::get();
    auto area = b.reduced (8.0f, 3.0f);

    g.setFont (AcidLookAndFeel::uiFont (9.5f, true));
    g.setColour (t.accent.withAlpha (0.8f));
    g.drawText (bankText.isEmpty() ? String ("PRESET") : bankText, area.removeFromTop (11.0f), Justification::centredLeft);

    g.setFont (AcidLookAndFeel::uiFont (16.0f, true));
    g.setColour (t.accent);
    g.drawFittedText (nameText, area.removeFromTop (19.0f).toNearestInt(), Justification::centredLeft, 1, 0.8f);

    g.setFont (AcidLookAndFeel::uiFont (11.0f));
    g.setColour (Colour (0xffd0d0d4));
    g.drawFittedText (helpText.isEmpty() ? String ("Hover a control for help. Click the name to browse presets.") : helpText,
                      area.toNearestInt(), Justification::centredLeft, 1, 0.6f);
}

//==============================================================================
void InfoLcd::setContent (const String& name, const String& value)
{
    if (name != nameText || value != valueText)
    {
        nameText = name;
        valueText = value;
        repaint();
    }
}

void InfoLcd::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (1.0f);
    AcidLookAndFeel::drawLcd (g, b);
    auto textArea = b.reduced (6.0f, 0.0f);
    const auto font = AcidLookAndFeel::uiFont (11.0f, true);
    g.setFont (font);
    const float valueWidth = GlyphArrangement::getStringWidth (font, valueText) + 4.0f;
    g.setColour (Colour (0xffc8c8cc));
    g.drawFittedText (nameText, textArea.withTrimmedRight (valueWidth).toNearestInt(), Justification::centredLeft, 1, 0.75f);
    g.setColour (Theme::get().accent);
    g.drawText (valueText, textArea, Justification::centredRight);
}

//==============================================================================
void WaveDisplay::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (1.0f);
    AcidLookAndFeel::drawLcd (g, b);
    const auto& t = Theme::get();

    g.setFont (AcidLookAndFeel::uiFont (9.0f, true));
    g.setColour (t.accent.withAlpha (0.7f));
    g.drawText (morph > 0.5f ? "SQUARE" : "SAW", b.reduced (6.0f, 3.0f), Justification::topLeft);

    auto plot = b.reduced (10.0f, 14.0f).withTrimmedBottom (-6.0f);
    g.setColour (Colours::white.withAlpha (0.1f));
    g.drawHorizontalLine ((int) plot.getCentreY(), plot.getX(), plot.getRight());

    Path wave;
    const int points = (int) plot.getWidth();
    for (int i = 0; i <= points; ++i)
    {
        const float ph = 2.0f * (float) i / (float) points;
        const float v = acid::Oscillator::shapeAt (ph + 0.5f, morph);
        const float x = plot.getX() + (float) i;
        const float y = plot.getCentreY() + v * plot.getHeight() * 0.42f;
        if (i == 0) wave.startNewSubPath (x, y); else wave.lineTo (x, y);
    }
    g.setColour (t.accentGlow.withAlpha (0.3f));
    g.strokePath (wave, PathStrokeType (3.5f));
    g.setColour (t.accent);
    g.strokePath (wave, PathStrokeType (1.4f));
}

//==============================================================================
void LfoPreview::setState (int newShape, float newPhase, float newDepth, bool newMirror)
{
    shape = newShape; phase = newPhase; depth = newDepth; mirror = newMirror;
    repaint();
}

void LfoPreview::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (1.0f);
    AcidLookAndFeel::drawLcd (g, b);
    const auto& t = Theme::get();

    g.setFont (AcidLookAndFeel::uiFont (9.0f, true));
    g.setColour (t.accent.withAlpha (0.7f));
    g.drawText ("SHAPE", b.reduced (8.0f, 4.0f), Justification::topLeft);
    g.drawText (acid::lfoShapeShort()[shape] + (mirror ? "  +" : ""), b.reduced (8.0f, 4.0f), Justification::topRight);

    auto plot = b.reduced (10.0f, 18.0f).withTrimmedBottom (-8.0f);
    g.setColour (Colours::white.withAlpha (0.1f));
    g.drawHorizontalLine ((int) plot.getCentreY(), plot.getX(), plot.getRight());
    for (int i = 1; i < 4; ++i)
        g.drawVerticalLine ((int) (plot.getX() + plot.getWidth() * (float) i / 4.0f), plot.getY(), plot.getBottom());

    // bipolar: around the centre line; mirrored (unipolar): up from the bottom
    const float sizeFrac = 0.35f + 0.65f * std::sqrt (std::abs (depth));
    const float sign = depth < 0.0f ? -1.0f : 1.0f;
    auto yFor = [&] (double ph)
    {
        const float v = sign * acid::Lfo::shapeValue (shape, ph);
        if (mirror)
            return plot.getBottom() - (0.5f * v + 0.5f) * plot.getHeight() * sizeFrac;
        return plot.getCentreY() - v * plot.getHeight() * 0.5f * sizeFrac;
    };

    Path wave;
    const int points = (int) plot.getWidth();
    for (int i = 0; i <= points; ++i)
    {
        const float x = plot.getX() + (float) i;
        const float y = yFor ((double) i / (double) points);
        if (i == 0) wave.startNewSubPath (x, y); else wave.lineTo (x, y);
    }
    g.setColour (t.accentGlow.withAlpha (0.3f));
    g.strokePath (wave, PathStrokeType (3.5f));
    g.setColour (t.accent);
    g.strokePath (wave, PathStrokeType (1.4f));

    const float px = plot.getX() + phase * plot.getWidth();
    const float py = yFor (phase);
    g.setColour (t.accentLight.withAlpha (0.35f));
    g.fillEllipse (px - 5.0f, py - 5.0f, 10.0f, 10.0f);
    g.setColour (Colours::white);
    g.fillEllipse (px - 2.5f, py - 2.5f, 5.0f, 5.0f);
}

//==============================================================================
void EnvMeter::setLevels (float env, float accent)
{
    if (std::abs (env - envLevel) > 0.005f || std::abs (accent - accentLevel) > 0.005f)
    {
        envLevel = env;
        accentLevel = accent;
        repaint();
    }
}

void EnvMeter::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (1.0f);
    AcidLookAndFeel::drawLcd (g, b);
    const auto& t = Theme::get();
    auto area = b.reduced (8.0f, 6.0f);

    auto bar = [&] (const String& name, float level, Rectangle<float> r)
    {
        g.setFont (AcidLookAndFeel::uiFont (9.0f, true));
        g.setColour (Colour (0xffc8c8cc));
        g.drawText (name, r.removeFromLeft (30.0f), Justification::centredLeft);
        auto track = r.reduced (0.0f, r.getHeight() * 0.3f);
        g.setColour (Colours::white.withAlpha (0.08f));
        g.fillRoundedRectangle (track, 2.0f);
        g.setGradientFill (ColourGradient (Colour (0xffff8a2a), 0.0f, track.getY(), Colour (0xffc4520a), 0.0f, track.getBottom(), false));
        g.fillRoundedRectangle (track.withWidth (track.getWidth() * jlimit (0.0f, 1.0f, level)), 2.0f);
    };

    bar ("ENV", envLevel, area.removeFromTop (area.getHeight() * 0.5f));
    bar ("ACC", accentLevel, area);
    ignoreUnused (t);
}

//==============================================================================
void HelpButton::paintButton (Graphics& g, bool isHighlighted, bool isDown)
{
    const auto& t = Theme::get();
    auto b = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (Colours::black.withAlpha (0.6f));
    g.fillEllipse (b.translated (0.0f, 1.0f));
    const auto top = isHighlighted ? t.buttonTop.brighter (0.12f) : t.buttonTop;
    const auto bottom = isHighlighted ? t.buttonBottom.brighter (0.12f) : t.buttonBottom;
    g.setGradientFill (ColourGradient (top, 0.0f, b.getY(), bottom, 0.0f, b.getBottom(), false));
    g.fillEllipse (isDown ? b.reduced (0.5f) : b);
    g.setColour (t.bevelDark);
    g.drawEllipse (b, 1.0f);
    g.setColour (t.buttonText);
    g.setFont (AcidLookAndFeel::uiFont (12.0f, true));
    g.drawText ("?", b, Justification::centred);
}

void HelpOverlay::paint (Graphics& g)
{
    g.fillAll (Colours::black.withAlpha (0.72f));
    auto box = getLocalBounds().toFloat().reduced (40.0f, 40.0f);
    AcidLookAndFeel::drawPanel (g, box, String (ACID_PRODUCT_NAME) + " - Quick Help");

    const char* lines[] =
    {
        "A:  the Acid synth - Osc 1, Filter, Envelope; Osc 2, Hard Sync / FM, Unison, Velocity and its Amp Envelope.",
        "ARPEGGIATOR:  Step mode plays the step sequence whenever the host plays (from the Key's root); keys you hold take over.",
        "    Arp off = the keys play the synth directly (overlapping notes slide). The Velocity lane sets each step's loudness.",
        "STEP SEQUENCER:  Volume (bottom = rest), Velocity, Transpose, Ratchet, Accent and Glide lanes. A Glide to a new pitch",
        "    glides over the GLIDE time. Length 16 / 32, End, Undo / Redo, Clear, Import (MIDI or audio) or drop a file on the grid.",
        "RANDOMISE:  Density, Range, Glide, Ratch, Acc, Vel Lo / Hi, with locks. Scale Lock keeps every step in the Key and Scale.",
        "B:  the 303, its own Amp Envelope and its options. A or B also chooses the synth that plays.",
        "FX:  Distortion (any number of the six modes), Delay and the Tube. The sequencer stays underneath A, B and FX.",
        "Amp Envelope Sync: note values from 1/64 to 1/1. Header: Delay and Dist switch those on / off.",
        "Knobs move in whole steps (drag or mouse wheel, one notch = 1). Double-click = zero. Drag the corner to resize.",
        "",
        "Click anywhere to close."
    };

    auto text = box.reduced (16.0f, 0.0f).withTrimmedTop (34.0f);
    g.setFont (AcidLookAndFeel::uiFont (12.0f));
    for (auto* line : lines)
    {
        g.setColour (String (line).startsWith ("Click") ? Theme::get().accent : Theme::get().text.withAlpha (0.9f));
        g.drawFittedText (line, text.removeFromTop (26.0f).toNearestInt(), Justification::centredLeft, 1, 0.8f);
    }
}

//==============================================================================
OctaveStepper::OctaveStepper (RangedAudioParameter& p, const String& helpName)
    : param (p), attachment (p, [this] (float v) { value = roundToInt (v); repaint(); })
{
    down.onClick = [this] { step (-1); };
    up.onClick = [this] { step (1); };
    setHelp (down, helpName + "  -  one octave down");
    setHelp (up, helpName + "  -  one octave up");
    setHelp (*this, helpName + "  -  octave up / down (arrows or mouse wheel, double-click = 0)");
    addAndMakeVisible (down);
    addAndMakeVisible (up);
    attachment.sendInitialUpdate();
}

void OctaveStepper::step (int delta)
{
    const auto range = param.getNormalisableRange();
    attachment.setValueAsCompleteGesture (jlimit (range.start, range.end, (float) (value + delta)));
}

void OctaveStepper::resized()
{
    auto b = getLocalBounds();
    const int arrow = jmin (24, getWidth() / 4);
    down.setBounds (b.removeFromLeft (arrow));
    up.setBounds (b.removeFromRight (arrow));
}

void OctaveStepper::paint (Graphics& g)
{
    const float arrow = (float) jmin (24, getWidth() / 4);
    auto b = getLocalBounds().toFloat().reduced (arrow + 2.0f, 0.0f);
    AcidLookAndFeel::drawLcd (g, b.reduced (0.0f, 1.0f));
    g.setFont (AcidLookAndFeel::uiFont (11.0f, true));
    const String number = (value > 0 ? "+" : "") + String (value);
    const bool roomy = b.getWidth() >= 54.0f;      // narrow: just the number, centred
    if (roomy)
    {
        g.setColour (Colour (0xffc8c8cc));
        g.drawText ("OCT", b.reduced (6.0f, 0.0f), Justification::centredLeft);
    }
    g.setColour (value == 0 ? Colour (0xffc8c8cc) : Theme::get().accent);
    g.drawText (number, b.reduced (roomy ? 6.0f : 0.0f, 0.0f), roomy ? Justification::centredRight : Justification::centred);
}

void OctaveStepper::mouseWheelMove (const MouseEvent&, const MouseWheelDetails& wheel)
{
    const float amount = std::abs (wheel.deltaY) > 0.0f ? wheel.deltaY : wheel.deltaX;
    if (std::abs (amount) > 1.0e-4f)
        step ((amount > 0.0f) != wheel.isReversed ? 1 : -1);
}

void OctaveStepper::mouseDoubleClick (const MouseEvent&)
{
    attachment.setValueAsCompleteGesture (0.0f);
}
