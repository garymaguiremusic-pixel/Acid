#include "Controls.h"

using namespace juce;

//==============================================================================
SegmentedButtons::SegmentedButtons (RangedAudioParameter& p, const StringArray& labels, const String& tooltip)
    : param (p),
      attachment (p, [this] (float v) { select (roundToInt (v) - minimum); })
{
    static int nextGroup = 9000;
    const int group = ++nextGroup;
    minimum = roundToInt (p.getNormalisableRange().start);

    for (int i = 0; i < labels.size(); ++i)
    {
        auto* b = buttons.add (new TextButton (labels[i]));
        b->setRadioGroupId (group, dontSendNotification);
        b->setClickingTogglesState (true);
        b->setTooltip (tooltip);
        b->setConnectedEdges ((i > 0 ? Button::ConnectedOnLeft : 0) | (i < labels.size() - 1 ? Button::ConnectedOnRight : 0));
        b->onClick = [this, i]
        {
            if (buttons[i]->getToggleState())
                attachment.setValueAsCompleteGesture ((float) (minimum + i));
        };
        addAndMakeVisible (b);
    }

    attachment.sendInitialUpdate();
}

void SegmentedButtons::select (int index)
{
    if (isPositiveAndBelow (index, buttons.size()))
        buttons[index]->setToggleState (true, dontSendNotification);
}

void SegmentedButtons::resized()
{
    const int n = buttons.size();
    const float w = (float) getWidth() / (float) jmax (1, n);
    for (int i = 0; i < n; ++i)
        buttons[i]->setBounds (roundToInt (i * w), 0, roundToInt ((i + 1) * w) - roundToInt (i * w) + (i < n - 1 ? 1 : 0), getHeight());
}

//==============================================================================
#include "ArpCompat.h"

SeedField::SeedField()
{
    setEditable (false, true, false);
    setJustificationType (Justification::centredRight);
    setFont (CustomLookAndFeel::uiFont (12.0f, true));
    setColour (Label::textColourId, Colours::white);
    setColour (Label::textWhenEditingColourId, Colours::white);
    setColour (Label::backgroundWhenEditingColourId, VirusColours::lcdBottom);
    setColour (TextEditor::highlightColourId, VirusColours::accentGlow);
    setTooltip ("Seed of the last randomise. Double-click and type a seed to regenerate that pattern.");
    setText ("1", dontSendNotification);

    onTextChange = [this]
    {
        const auto value = (uint32) jmax ((int64) 1, getText().retainCharacters ("0123456789").getLargeIntValue());
        setSeed (value);
        if (onSeedEntered)
            onSeedEntered (value);
    };
}

void SeedField::setSeed (uint32 newSeed)
{
    seed = newSeed;
    setText (String (seed), dontSendNotification);
    repaint();
}

void SeedField::paint (Graphics& g)
{
    if (isBeingEdited())
        return;

    auto b = getLocalBounds().toFloat().reduced (1.0f);
    CustomLookAndFeel::drawLcd (g, b, false);
    auto text = b.reduced (6.0f, 0.0f);
    g.setFont (CustomLookAndFeel::uiFont (11.0f, true));
    g.setColour (Colours::white);
    g.drawText ("SEED", text, Justification::centredLeft);
    g.drawText (String (seed), text, Justification::centredRight);
}

//==============================================================================
void ArrowIconButton::paintButton (Graphics& g, bool isHighlighted, bool isDown)
{
    getLookAndFeel().drawButtonBackground (g, *this, findColour (TextButton::buttonColourId), isHighlighted, isDown);

    auto b = getLocalBounds().toFloat().reduced (getWidth() * 0.32f, getHeight() * 0.3f);
    Path arrow;
    if (right)
        arrow.addTriangle (b.getX(), b.getY(), b.getRight(), b.getCentreY(), b.getX(), b.getBottom());
    else
        arrow.addTriangle (b.getRight(), b.getY(), b.getX(), b.getCentreY(), b.getRight(), b.getBottom());

    g.setColour (VirusColours::buttonText.withMultipliedAlpha (isEnabled() ? 1.0f : 0.4f));
    g.fillPath (arrow);
}
