#include "Knob.h"
#include "LookAndFeel.h"
#include "../Parameters.h"

using namespace juce;

AcidKnob::AcidKnob (Size s, const String& labelText)
    : Slider (Slider::RotaryVerticalDrag, Slider::NoTextBox), knobSize (s), label (labelText)
{
    setRange (0.0, 100.0, 1.0);
    setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
    setName (labelText);
}

String AcidKnob::valueText()
{
    return displayValue ? displayValue (getValue()) : acid::formatNumber (getValue());
}

String AcidKnob::getHelpLine()
{
    String text = label.toUpperCase() + "  " + valueText();
    if (helpText.isNotEmpty())
        text << "   -   " << helpText;
    return text;
}

void AcidKnob::paint (Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    const float pos = (float) valueToProportionOfLength (getValue());
    const auto rotary = getRotaryParameters();

    Rectangle<float> knobArea;
    if (knobSize == Size::Large)
    {
        knobArea = bounds;
    }
    else
    {
        auto labelArea = bounds.removeFromBottom (14.0f);
        const float side = std::min (bounds.getWidth(), bounds.getHeight());
        knobArea = bounds.withSizeKeepingCentre (side, side);

        if ((hovering || dragging) && isEnabled())
        {
            g.setFont (AcidLookAndFeel::uiFont (11.0f, true));
            g.setColour (Theme::get().accent);
            g.drawFittedText (valueText(), labelArea.toNearestInt(), Justification::centred, 1, 0.6f);
        }
        else
        {
            AcidLookAndFeel::drawSmallLabel (g, label, labelArea);
        }
    }

    getLookAndFeel().drawRotarySlider (g, (int) knobArea.getX(), (int) knobArea.getY(),
                                       (int) knobArea.getWidth(), (int) knobArea.getHeight(),
                                       pos, rotary.startAngleRadians, rotary.endAngleRadians, *this);
}

void AcidKnob::setValueWithGesture (double v)
{
    if (onGestureBegin) onGestureBegin();
    setValue (jlimit (getMinimum(), getMaximum(), v), sendNotificationSync);
    if (onGestureEnd) onGestureEnd();
}

void AcidKnob::mouseDown (const MouseEvent& e)
{
    if (! isEnabled())
        return;
    dragging = true;
    lastDragY = e.position.y;
    dragValue = getValue();
    if (onGestureBegin) onGestureBegin();
    repaint();
}

void AcidKnob::mouseDrag (const MouseEvent& e)
{
    if (! isEnabled() || ! dragging)
        return;

    const double pixels = (double) (lastDragY - e.position.y);
    lastDragY = e.position.y;

    // full range in about 200 pixels (small ranges: one step per 12 pixels), always whole steps
    const double range = getMaximum() - getMinimum();
    const double perPixel = std::max (range / 200.0, 1.0 / 12.0);
    dragValue = jlimit (getMinimum(), getMaximum(), dragValue + pixels * perPixel);

    const double snapped = std::round (dragValue);
    if (std::abs (snapped - getValue()) > 1.0e-9)
        setValue (snapped, sendNotificationSync);
}

void AcidKnob::mouseUp (const MouseEvent&)
{
    if (! dragging)
        return;
    dragging = false;
    if (onGestureEnd) onGestureEnd();
    repaint();
}

void AcidKnob::mouseDoubleClick (const MouseEvent&)
{
    if (isEnabled())
        setValueWithGesture (defaultValue);
}

void AcidKnob::mouseWheelMove (const MouseEvent&, const MouseWheelDetails& wheel)
{
    if (! isEnabled())
        return;

    const float dir = (wheel.isReversed ? -1.0f : 1.0f) * (std::abs (wheel.deltaY) > 0.0f ? wheel.deltaY : wheel.deltaX);
    if (std::abs (dir) < 1.0e-6f)
        return;

    // one notch = one step of 1.0
    setValueWithGesture (std::round (getValue()) + (dir > 0.0f ? 1.0 : -1.0));
}

void AcidKnob::mouseEnter (const MouseEvent&) { hovering = true;  repaint(); }
void AcidKnob::mouseExit (const MouseEvent&)  { hovering = false; repaint(); }

//==============================================================================
KnobAttachment::KnobAttachment (RangedAudioParameter& p, AcidKnob& k)
    : knob (k),
      attachment (p, [this] (float v) { knob.setValue (v, dontSendNotification); knob.repaint(); })
{
    const auto range = p.getNormalisableRange();
    knob.setRange (range.start, range.end, 1.0);
    knob.setDefaultValue (p.convertFrom0to1 (p.getDefaultValue()));
    knob.setBipolar (range.start < 0.0f && range.end > 0.0f);
    if (knob.helpText.isEmpty())
        knob.helpText = acid::helpForParameter (p.getParameterID());
    if (! knob.displayValue)
    {
        const auto id = p.getParameterID();
        knob.displayValue = [id] (double v) { return acid::describeParameter (id, (float) v); };
    }

    knob.onGestureBegin = [this] { attachment.beginGesture(); };
    knob.onGestureEnd   = [this] { attachment.endGesture(); };
    knob.onValueChange  = [this] { attachment.setValueAsPartOfGesture ((float) knob.getValue()); };

    attachment.sendInitialUpdate();
}

KnobAttachment::~KnobAttachment()
{
    knob.onGestureBegin = nullptr;
    knob.onGestureEnd = nullptr;
    knob.onValueChange = nullptr;
}
