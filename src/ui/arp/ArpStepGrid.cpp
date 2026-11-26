#include "ArpStepGrid.h"
#include "ArpCompat.h"

using namespace juce;
using as::arp::Pattern;

//==============================================================================
ArpStepGrid::ArpStepGrid (as::arp::PatternModel& m, RangedAudioParameter& lengthParam, RangedAudioParameter& endParam)
    : model (m),
      lengthAttachment (lengthParam, [this] (float v) { length = roundToInt (v) == 0 ? 16 : 32; repaint(); }),
      endAttachment (endParam, [this] (float v) { endStep = roundToInt (v); repaint(); })
{
    pattern = model.get();
    model.addChangeListener (this);
    lengthAttachment.sendInitialUpdate();
    endAttachment.sendInitialUpdate();
}

ArpStepGrid::~ArpStepGrid()
{
    model.removeChangeListener (this);
}

void ArpStepGrid::changeListenerCallback (ChangeBroadcaster*)
{
    pattern = model.get();
    repaint();
}

void ArpStepGrid::setCurrentStep (int step)
{
    if (step != currentStep)
    {
        currentStep = step;
        repaint();
    }
}

//==============================================================================
ArpStepGrid::Layout ArpStepGrid::getLayout() const
{
    Layout l;
    auto b = getLocalBounds().toFloat();
    l.labels = b.removeFromLeft (74.0f);
    l.gridX = b.getX();
    l.stepWidth = b.getWidth() / (float) numVisibleSteps();

    l.header = b.removeFromTop (15.0f);
    b.removeFromTop (3.0f);
    l.glide = b.removeFromBottom (18.0f);
    b.removeFromBottom (3.0f);
    l.accent = b.removeFromBottom (18.0f);
    b.removeFromBottom (3.0f);
    l.ratchet = b.removeFromBottom (18.0f);
    b.removeFromBottom (3.0f);
    l.transpose = b.removeFromBottom (24.0f);
    b.removeFromBottom (3.0f);
    l.velocity = b.removeFromBottom (22.0f);
    b.removeFromBottom (3.0f);
    l.volume = b;
    return l;
}

Rectangle<float> ArpStepGrid::cell (Rectangle<float> lane, int step) const
{
    const auto l = getLayout();
    return { l.gridX + step * l.stepWidth, lane.getY(), l.stepWidth, lane.getHeight() };
}

int ArpStepGrid::stepAt (float x) const
{
    const auto l = getLayout();
    return jlimit (0, numVisibleSteps() - 1, (int) std::floor ((x - l.gridX) / l.stepWidth));
}

ArpStepGrid::Lane ArpStepGrid::laneAt (Point<float> p) const
{
    const auto l = getLayout();
    if (p.x < l.gridX)               return Lane::None;
    if (l.volume.contains (p))       return Lane::Volume;
    if (l.velocity.contains (p))     return Lane::Velocity;
    if (l.transpose.contains (p))    return Lane::Transpose;
    if (l.glide.contains (p))        return Lane::Glide;
    if (l.ratchet.contains (p))      return Lane::Ratchet;
    if (l.accent.contains (p))       return Lane::Accent;
    if (l.header.contains (p))       return Lane::Header;
    return Lane::None;
}

int ArpStepGrid::valueFromY (float y, Rectangle<float> laneArea, int minimum)
{
    const auto lane = laneArea.reduced (0.0f, 3.0f);
    return jlimit (minimum, 127, roundToInt ((lane.getBottom() - y) / lane.getHeight() * 127.0f));
}

int ArpStepGrid::applyQuantise (int transpose) const
{
    transpose = jlimit (-24, 24, transpose);
    return quantiseTranspose ? jlimit (-24, 24, quantiseTranspose (transpose)) : transpose;
}

//==============================================================================
void ArpStepGrid::paint (Graphics& g)
{
    const auto l = getLayout();
    const int playLength = as::arp::effectiveLength (length, endStep);

    // lane labels
    CustomLookAndFeel::drawSmallLabel (g, "Volume", l.labels.withY (l.volume.getY()).withHeight (l.volume.getHeight()), Justification::centredLeft);
    CustomLookAndFeel::drawSmallLabel (g, "Velocity", l.labels.withY (l.velocity.getY()).withHeight (l.velocity.getHeight()), Justification::centredLeft);
    CustomLookAndFeel::drawSmallLabel (g, "Transpose", l.labels.withY (l.transpose.getY()).withHeight (l.transpose.getHeight()), Justification::centredLeft);
    CustomLookAndFeel::drawSmallLabel (g, "Ratchet", l.labels.withY (l.ratchet.getY()).withHeight (l.ratchet.getHeight()), Justification::centredLeft);
    CustomLookAndFeel::drawSmallLabel (g, "Accent", l.labels.withY (l.accent.getY()).withHeight (l.accent.getHeight()), Justification::centredLeft);
    CustomLookAndFeel::drawSmallLabel (g, "Glide", l.labels.withY (l.glide.getY()).withHeight (l.glide.getHeight()), Justification::centredLeft);

    // lane backgrounds
    CustomLookAndFeel::drawInsetBox (g, l.volume);
    CustomLookAndFeel::drawInsetBox (g, l.velocity);
    CustomLookAndFeel::drawInsetBox (g, l.transpose);
    CustomLookAndFeel::drawInsetBox (g, l.ratchet);
    CustomLookAndFeel::drawInsetBox (g, l.accent);

    // guide lines in the volume lane at 25/50/75 %
    g.setColour (Colours::white.withAlpha (0.05f));
    for (int i = 1; i < 4; ++i)
    {
        const float y = l.volume.getBottom() - l.volume.getHeight() * (float) i / 4.0f;
        g.drawHorizontalLine (roundToInt (y), l.volume.getX() + 2.0f, l.volume.getRight() - 2.0f);
    }

    for (int i = 0; i < numVisibleSteps(); ++i)
    {
        const auto& s = pattern.steps[(size_t) i];
        const bool active = i < playLength;
        const bool playing = i == currentStep;
        const float alpha = active ? 1.0f : 0.3f;

        // beat separators every 4 steps
        if (i > 0)
        {
            const float x = l.gridX + i * l.stepWidth;
            g.setColour (Colours::black.withAlpha (i % 4 == 0 ? 0.7f : 0.3f));
            g.drawVerticalLine (roundToInt (x), l.volume.getY() + 2.0f, l.volume.getBottom() - 2.0f);
            g.drawVerticalLine (roundToInt (x), l.velocity.getY() + 2.0f, l.velocity.getBottom() - 2.0f);
        }

        // header: number + LED
        {
            auto h = cell (l.header, i).reduced (1.0f, 0.0f);
            if (playing)
            {
                g.setColour (VirusColours::accent.withAlpha (0.9f));          // the playing step
                g.fillRoundedRectangle (h.reduced (2.0f, 2.0f), 2.0f);
            }
            g.setColour ((playing ? Colours::white : (i % 4 == 0 ? VirusColours::headerText : VirusColours::labelText)).withMultipliedAlpha (alpha));
            g.setFont (CustomLookAndFeel::uiFont (10.0f, i % 4 == 0 || playing));
            g.drawText (String (i + 1), h, Justification::centred);
        }

        // volume bar
        {
            auto c = cell (l.volume, i).reduced (l.stepWidth > 30.0f ? 4.0f : 2.0f, 3.0f);
            if (! s.isRest())
            {
                auto bar = c.withTop (c.getBottom() - c.getHeight() * (float) s.volume / 127.0f);
                const auto top = (playing ? VirusColours::accentLight : VirusColours::buttonOnTop).withMultipliedAlpha (alpha);
                const auto bottom = (playing ? VirusColours::accent : VirusColours::buttonOnBottom).withMultipliedAlpha (alpha);
                g.setGradientFill (ColourGradient (top, 0.0f, bar.getY(), bottom, 0.0f, bar.getBottom(), false));
                g.fillRoundedRectangle (bar, 1.5f);
                g.setColour (Colours::white.withAlpha (0.5f * alpha));
                g.drawHorizontalLine (roundToInt (bar.getY()), bar.getX() + 1.0f, bar.getRight() - 1.0f);
            }
            else
            {
                // rest marker
                g.setColour (Theme::get().text.withAlpha (0.25f * alpha));
                g.fillRect (c.withTop (c.getBottom() - 2.0f).reduced (c.getWidth() * 0.25f, 0.0f));
            }

            if (playing)
            {
                g.setColour (Colours::white.withAlpha (0.06f));
                g.fillRect (cell (l.volume, i).reduced (1.0f, 2.0f));
            }
        }

        // velocity bar (thin, lighter; dimmed on rests)
        {
            auto c = cell (l.velocity, i).reduced (l.stepWidth > 30.0f ? 8.0f : 4.0f, 3.0f);
            auto bar = c.withTop (c.getBottom() - c.getHeight() * (float) jlimit (0, 127, s.velocity) / 127.0f);
            g.setColour (VirusColours::accentLight.withMultipliedAlpha (alpha * (s.isRest() ? 0.25f : (playing ? 1.0f : 0.75f))));
            g.fillRoundedRectangle (bar, 1.0f);
        }

        // transpose value
        {
            auto c = cell (l.transpose, i);
            const String text = s.transpose == 0 ? String ("0") : (s.transpose > 0 ? "+" : "") + String (s.transpose);
            g.setFont (CustomLookAndFeel::uiFont (l.stepWidth > 30.0f ? 12.0f : 10.0f, true));
            g.setColour ((s.transpose == 0 ? VirusColours::labelText.withAlpha (0.45f) : Theme::get().text).withMultipliedAlpha (alpha));
            g.drawText (text, c, Justification::centred);
            if (i > 0)
            {
                g.setColour (Colours::black.withAlpha (0.5f));
                g.drawVerticalLine (roundToInt (c.getX()), c.getY() + 3.0f, c.getBottom() - 3.0f);
            }
        }

        // ratchet: one tick per hit
        {
            auto c = cell (l.ratchet, i);
            const int hits = jlimit (1, as::arp::kMaxRatchet, s.ratchet);
            const float tickW = jmin (4.0f, (c.getWidth() - 8.0f) / 7.0f);
            const float gap = tickW * 0.8f;
            const float totalW = hits * tickW + (hits - 1) * gap;
            float x = c.getCentreX() - totalW * 0.5f;
            g.setColour (hits > 1 ? (playing ? VirusColours::accentLight : VirusColours::accent).withMultipliedAlpha (alpha)
                                  : Theme::get().text.withAlpha (0.16f * alpha));
            for (int h = 0; h < hits; ++h, x += tickW + gap)
                g.fillRoundedRectangle (x, c.getY() + 5.0f, tickW, c.getHeight() - 10.0f, 1.0f);

            if (i > 0)
            {
                g.setColour (Colours::black.withAlpha (0.5f));
                g.drawVerticalLine (roundToInt (c.getX()), c.getY() + 3.0f, c.getBottom() - 3.0f);
            }
        }

        // accent: a bold ">" mark (lit = accented)
        {
            auto c = cell (l.accent, i);
            const float size = jmin (c.getHeight() - 6.0f, c.getWidth() - 6.0f);
            auto r = c.withSizeKeepingCentre (size * 0.8f, size);
            Path mark;
            mark.startNewSubPath (r.getX(), r.getY());
            mark.lineTo (r.getRight(), r.getCentreY());
            mark.lineTo (r.getX(), r.getBottom());

            if (s.accent)
            {
                g.setColour (VirusColours::accentGlow.withAlpha (0.5f * alpha));   // white mark, orange glow
                g.strokePath (mark, PathStrokeType (5.0f, PathStrokeType::mitered, PathStrokeType::rounded));
                g.setColour (Colours::white.withMultipliedAlpha (alpha));
            }
            else
            {
                g.setColour (Theme::get().text.withAlpha (0.14f * alpha));
            }
            g.strokePath (mark, PathStrokeType (2.2f, PathStrokeType::mitered, PathStrokeType::rounded));

            if (i > 0)
            {
                g.setColour (Colours::black.withAlpha (0.5f));
                g.drawVerticalLine (roundToInt (c.getX()), c.getY() + 3.0f, c.getBottom() - 3.0f);
            }
        }

        // glide: a button per step, like the first Acid sequencer's slide row (lit = slides into the next step)
        {
            auto c = cell (l.glide, i).reduced (2.0f, 0.0f);
            const auto& t = Theme::get();
            g.setGradientFill (ColourGradient (s.glide ? Colour (0xffff8a2a) : t.buttonTop, 0.0f, c.getY(),
                                               s.glide ? Colour (0xffc4520a) : t.buttonBottom, 0.0f, c.getBottom(), false));
            g.fillRoundedRectangle (c, 2.5f);
            g.setColour (t.bevelDark);
            g.drawRoundedRectangle (c, 2.5f, 1.0f);
            auto dash = c.withSizeKeepingCentre (c.getWidth() * 0.42f, 2.5f);
            g.setColour (s.glide ? Colours::white.withMultipliedAlpha (alpha) : t.labelText.withAlpha (0.3f * alpha));
            g.fillRoundedRectangle (dash, 1.0f);
            if (! active)
            {
                g.setColour (Colours::black.withAlpha (Theme::isLight() ? 0.15f : 0.45f));
                g.fillRoundedRectangle (c, 2.5f);
            }
        }

    }

    // End marker
    if (endStep > 0 && endStep < numVisibleSteps())
    {
        const float x = l.gridX + endStep * l.stepWidth;
        g.setColour (Colours::white.withAlpha (0.8f));                  // End marker
        g.drawLine (x, l.header.getY(), x, l.glide.getBottom(), 2.0f);
    }

    // hover outline
    if (hoverStep >= 0 && hoverLane != Lane::None && hoverLane != Lane::Header)
    {
        const auto lane = hoverLane == Lane::Volume ? l.volume
                        : hoverLane == Lane::Velocity ? l.velocity
                        : hoverLane == Lane::Transpose ? l.transpose
                        : hoverLane == Lane::Ratchet ? l.ratchet
                        : hoverLane == Lane::Accent ? l.accent
                        : l.glide;
        g.setColour (VirusColours::accent.withAlpha (0.4f));
        g.drawRect (cell (lane, hoverStep).reduced (1.0f), 1.0f);
    }

    // a short message (what was imported)
    if (status.isNotEmpty() && dropHint.isEmpty())
    {
        g.setFont (CustomLookAndFeel::uiFont (12.5f, true));
        const float w = jmin (getWidth() - l.labels.getWidth() - 20.0f, GlyphArrangement::getStringWidth (g.getCurrentFont(), status) + 28.0f);
        auto box = Rectangle<float> (w, 24.0f).withCentre ({ l.gridX + (getWidth() - l.gridX) * 0.5f, l.velocity.getCentreY() });
        g.setColour (VirusColours::background.withAlpha (0.92f));
        g.fillRoundedRectangle (box, 5.0f);
        g.setColour (VirusColours::accent);
        g.drawRoundedRectangle (box.reduced (0.5f), 5.0f, 1.0f);
        g.setColour (Colours::white);
        g.drawText (status, box, Justification::centred, true);
    }

    // a file is being dragged over: show what dropping it will do
    if (dropHint.isNotEmpty())
    {
        auto area = getLocalBounds().toFloat().withTrimmedLeft (l.labels.getWidth()).reduced (2.0f);
        g.setColour (VirusColours::background.withAlpha (0.82f));
        g.fillRoundedRectangle (area, 6.0f);
        Path outline;
        outline.addRoundedRectangle (area.reduced (1.0f), 6.0f);
        const float dashes[] = { 8.0f, 6.0f };
        PathStrokeType (2.0f).createDashedStroke (outline, outline, dashes, 2);
        g.setColour (VirusColours::accent);
        g.fillPath (outline);

        const bool isMidi = dropHint.endsWithIgnoreCase (".mid") || dropHint.endsWithIgnoreCase (".midi");
        auto text = area.withSizeKeepingCentre (area.getWidth(), 64.0f);
        g.setFont (CustomLookAndFeel::uiFont (17.0f, true));
        g.setColour (Colours::white);
        g.drawText ("Drop to import  " + dropHint, text.removeFromTop (28.0f), Justification::centred, true);
        g.setFont (CustomLookAndFeel::uiFont (12.5f));
        g.setColour (VirusColours::labelText);
        g.drawText (isMidi ? "MIDI notes only (CC, pitch bend and other messages are ignored) - fills the steps from bar 1"
                           : "Detects the notes of a single-note line (bass, acid, lead) and fills the steps",
                    text, Justification::centred, true);
    }
}

bool ArpStepGrid::isInterestedInFileDrag (const StringArray& files)
{
    return files.size() == 1 && canImport && canImport (File (files[0]));
}

void ArpStepGrid::fileDragEnter (const StringArray& files, int, int)
{
    setDropHint (File (files[0]).getFileName());
}

void ArpStepGrid::fileDragExit (const StringArray&)
{
    setDropHint ({});
}

void ArpStepGrid::filesDropped (const StringArray& files, int, int)
{
    setDropHint ({});
    if (onFileDropped && files.size() == 1)
        onFileDropped (File (files[0]));
}

void ArpStepGrid::showStatus (const String& message)
{
    status = message;
    repaint();
    if (message.isNotEmpty())
        startTimer (4000);
}

void ArpStepGrid::timerCallback()
{
    stopTimer();
    status.clear();
    repaint();
}

void ArpStepGrid::setDropHint (const String& fileName)
{
    if (fileName != dropHint)
    {
        dropHint = fileName;
        repaint();
    }
}

//==============================================================================
void ArpStepGrid::mouseMove (const MouseEvent& e)
{
    const auto lane = laneAt (e.position);
    const int step = lane == Lane::None ? -1 : stepAt (e.position.x);
    if (lane != hoverLane || step != hoverStep)
    {
        hoverLane = lane;
        hoverStep = step;
        setMouseCursor (lane == Lane::Transpose ? MouseCursor::UpDownResizeCursor : MouseCursor::NormalCursor);
        repaint();
    }
}

void ArpStepGrid::mouseDown (const MouseEvent& e)
{
    dragLane = laneAt (e.position);
    if (dragLane == Lane::None || dragLane == Lane::Header)
        return;

    dragStep = stepAt (e.position.x);
    auto& s = pattern.steps[(size_t) dragStep];
    model.pushUndoPoint();

    switch (dragLane)
    {
        case Lane::Volume:
            if (e.mods.isPopupMenu() || e.mods.isAltDown())
            {
                s.volume = s.volume > 0 ? 0 : 127;
                if (s.velocity <= 0)
                    s.velocity = 100;
                dragLane = Lane::None;          // a toggle, not a drag
            }
            else
            {
                s.volume = valueFromY (e.position.y, getLayout().volume, 0);
            }
            break;

        case Lane::Velocity:
            s.velocity = valueFromY (e.position.y, getLayout().velocity, 1);
            break;

        case Lane::Transpose:
            dragStartTranspose = s.transpose;
            dragStartY = e.position.y;
            break;

        case Lane::Reverse:
            break;

        case Lane::Accent:
            accentPaintValue = ! s.accent;
            s.accent = accentPaintValue;
            break;

        case Lane::Glide:
            glidePaintValue = ! s.glide;
            s.glide = glidePaintValue;
            break;

        case Lane::Ratchet:
        {
            const int n = as::arp::kMaxRatchet;
            const int current = jlimit (1, n, s.ratchet);
            ratchetPaintValue = e.mods.isShiftDown() ? (current == 1 ? n : current - 1) : (current % n) + 1;
            s.ratchet = ratchetPaintValue;
            break;
        }

        case Lane::None:
        case Lane::Header:
            break;
    }

    model.set (pattern, false);
}

void ArpStepGrid::mouseDrag (const MouseEvent& e)
{
    if (dragLane == Lane::None || dragLane == Lane::Header)
        return;

    switch (dragLane)
    {
        case Lane::Volume:
            pattern.steps[(size_t) stepAt (e.position.x)].volume = valueFromY (e.position.y, getLayout().volume, 0);
            break;

        case Lane::Velocity:
            pattern.steps[(size_t) stepAt (e.position.x)].velocity = valueFromY (e.position.y, getLayout().velocity, 1);
            break;

        case Lane::Transpose:
        {
            const float pixelsPerSemitone = e.mods.isShiftDown() ? 14.0f : 6.0f;
            const int delta = roundToInt ((dragStartY - e.position.y) / pixelsPerSemitone);
            pattern.steps[(size_t) dragStep].transpose = applyQuantise (dragStartTranspose + delta);
            break;
        }

        case Lane::Ratchet:
            pattern.steps[(size_t) stepAt (e.position.x)].ratchet = ratchetPaintValue;
            break;

        case Lane::Reverse:
            break;

        case Lane::Accent:
            pattern.steps[(size_t) stepAt (e.position.x)].accent = accentPaintValue;
            break;

        case Lane::Glide:
            pattern.steps[(size_t) stepAt (e.position.x)].glide = glidePaintValue;
            break;

        case Lane::None:
        case Lane::Header:
            break;
    }

    model.set (pattern, false);
}

void ArpStepGrid::mouseUp (const MouseEvent&)
{
    dragLane = Lane::None;
    dragStep = -1;
}

void ArpStepGrid::mouseDoubleClick (const MouseEvent& e)
{
    const auto lane = laneAt (e.position);
    const int step = stepAt (e.position.x);

    if (lane == Lane::Transpose)
        pattern.steps[(size_t) step].transpose = applyQuantise (0);
    else if (lane == Lane::Volume)
        pattern.steps[(size_t) step].volume = 127;
    else if (lane == Lane::Velocity)
        pattern.steps[(size_t) step].velocity = 100;
    else if (lane == Lane::Ratchet)
        pattern.steps[(size_t) step].ratchet = 1;
    else
        return;

    model.set (pattern, true);
}

void ArpStepGrid::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& wheel)
{
    const auto lane = laneAt (e.position);
    if (lane != Lane::Transpose && lane != Lane::Ratchet)
        return;

    const float amount = std::abs (wheel.deltaY) > 0.0f ? wheel.deltaY : wheel.deltaX;
    if (std::abs (amount) < 1.0e-4f)
        return;

    const int dir = (amount > 0.0f) != wheel.isReversed ? 1 : -1;
    auto& s = pattern.steps[(size_t) stepAt (e.position.x)];

    if (lane == Lane::Ratchet)
    {
        s.ratchet = jlimit (1, as::arp::kMaxRatchet, s.ratchet + dir);
        model.set (pattern, true);
        return;
    }
    int next = jlimit (-24, 24, s.transpose + dir);

    // with scale lock, keep moving until the value lands on a new scale note
    while (applyQuantise (next) == s.transpose && next > -24 && next < 24)
        next += dir;

    s.transpose = applyQuantise (next);
    model.set (pattern, true);
}

void ArpStepGrid::mouseExit (const MouseEvent&)
{
    if (hoverLane != Lane::None || hoverStep >= 0)
    {
        hoverLane = Lane::None;
        hoverStep = -1;
        repaint();
    }
}

String ArpStepGrid::getHoverHelp() const
{
    if (hoverStep < 0)
        return "STEP SEQUENCER  -  drop a MIDI or audio file here to import it";

    const auto& s = pattern.steps[(size_t) hoverStep];
    const String step = "STEP " + String (hoverStep + 1) + "  ";
    switch (hoverLane)
    {
        case Lane::Volume:    return step + "VOLUME " + String (s.volume) + "  -  drag to draw, bottom = rest, right-click toggles a rest, double-click = full";
        case Lane::Velocity:  return step + "VELOCITY " + String (s.velocity) + "  -  drag to draw (each step's loudness, Vel Link and MIDI Out), double-click = 100";
        case Lane::Transpose: return step + "TRANSPOSE " + (s.transpose > 0 ? "+" : "") + String (s.transpose)
                                     + (pattern.scaleSteps ? " scale steps" : " st") + "  -  drag up/down or scroll, double-click = 0";
        case Lane::Ratchet:   return step + "RATCHET x" + String (s.ratchet) + "  -  click to cycle 1-4 hits, shift-click back, scroll";
        case Lane::Accent:    return step + "ACCENT " + (s.accent ? "on" : "off") + "  -  louder, with the filter kick and accent sweep";
        case Lane::Glide:     return step + "GLIDE " + (s.glide ? "on" : "off") + "  -  slide into the next step's pitch over the Glide time (303 slide)";
        case Lane::Header:    return step + "-  the playing step lights up";
        case Lane::Reverse:
        case Lane::None:      break;
    }
    return "STEP SEQUENCER";
}
