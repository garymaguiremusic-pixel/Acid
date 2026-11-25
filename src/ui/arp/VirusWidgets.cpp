#include "VirusWidgets.h"
#include "ArpCompat.h"

using namespace juce;

void LcdDisplay::setContent (const String& name, const String& value)
{
    if (name != nameText || value != valueText)
    {
        nameText = name;
        valueText = value;
        repaint();
    }
}

void LcdDisplay::setDimmed (bool shouldDim)
{
    if (dimmed != shouldDim)
    {
        dimmed = shouldDim;
        repaint();
    }
}

void LcdDisplay::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (1.0f);
    CustomLookAndFeel::drawLcd (g, b, dimmed);

    const auto textColour = dimmed ? VirusColours::dimText : Colour (0xffc8c8cc);
    const auto valueColour = dimmed ? VirusColours::dimText : VirusColours::accent;
    auto textArea = b.reduced (6.0f, 0.0f);

    g.setFont (CustomLookAndFeel::uiFont (11.0f, true));
    g.setColour (Colours::black.withAlpha (0.35f));
    g.drawText (nameText, textArea.translated (0.0f, 1.0f), Justification::centredLeft);
    g.drawText (valueText, textArea.translated (0.0f, 1.0f), Justification::centredRight);
    g.setColour (textColour);
    g.drawText (nameText, textArea, Justification::centredLeft);
    g.setColour (valueColour);
    g.drawText (valueText, textArea, Justification::centredRight);
}

//==============================================================================
TempoField::TempoField (RangedAudioParameter& p)
    : param (p),
      attachment (p, [this] (float v) { internalBpm = v; repaint(); })
{
    attachment.sendInitialUpdate();
    setMouseCursor (MouseCursor::UpDownResizeCursor);
}

void TempoField::setTempo (float bpm, bool fromHost)
{
    if (std::abs (bpm - shownBpm) > 0.01f || fromHost != hostTempo)
    {
        shownBpm = bpm;
        hostTempo = fromHost;
        getProperties().set ("help", fromHost ? "TEMPO  -  provided by the host"
                                              : "TEMPO  -  internal tempo (no host tempo). Drag or scroll in whole BPM, double-click = 120");
        setMouseCursor (fromHost ? MouseCursor::NormalCursor : MouseCursor::UpDownResizeCursor);
        repaint();
    }
}

void TempoField::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    CustomLookAndFeel::drawInsetBox (g, b);

    auto tag = b.removeFromLeft (38.0f).reduced (3.0f);
    g.setColour (hostTempo ? VirusColours::popupHighlight : VirusColours::buttonOffTop);
    g.fillRoundedRectangle (tag, 2.0f);
    g.setColour (hostTempo ? Colours::white : VirusColours::buttonText);
    g.setFont (CustomLookAndFeel::uiFont (9.5f, true));
    g.drawText (hostTempo ? "HOST" : "INT", tag, Justification::centred);

    g.setColour (Theme::get().text);
    g.setFont (CustomLookAndFeel::uiFont (12.0f, true));
    g.drawText (String (hostTempo ? shownBpm : internalBpm, 1) + " BPM", b.reduced (4.0f, 0.0f), Justification::centredRight);
}

void TempoField::mouseDown (const MouseEvent& e)
{
    if (hostTempo)
        return;
    dragging = true;
    lastY = e.position.y;
    dragBpm = internalBpm;
    attachment.beginGesture();
}

void TempoField::mouseDrag (const MouseEvent& e)
{
    if (! dragging)
        return;
    dragBpm += (lastY - e.position.y) * 0.25f;      // one BPM every 4 pixels, whole steps
    lastY = e.position.y;
    const auto range = param.getNormalisableRange();
    dragBpm = jlimit (range.start, range.end, dragBpm);
    attachment.setValueAsPartOfGesture (std::round (dragBpm));
}

void TempoField::mouseUp (const MouseEvent&)
{
    if (dragging)
        attachment.endGesture();
    dragging = false;
}

void TempoField::mouseDoubleClick (const MouseEvent&)
{
    if (! hostTempo)
        attachment.setValueAsCompleteGesture (param.convertFrom0to1 (param.getDefaultValue()));
}

void TempoField::mouseWheelMove (const MouseEvent&, const MouseWheelDetails& wheel)
{
    if (hostTempo)
        return;
    const float amount = std::abs (wheel.deltaY) > 0.0f ? wheel.deltaY : wheel.deltaX;
    if (std::abs (amount) < 1.0e-4f)
        return;
    const auto range = param.getNormalisableRange();
    const float next = jlimit (range.start, range.end, std::round (internalBpm) + ((amount > 0.0f) != wheel.isReversed ? 1.0f : -1.0f));
    attachment.setValueAsCompleteGesture (next);
}
