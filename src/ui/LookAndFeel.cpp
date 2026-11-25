#include "LookAndFeel.h"
#include "Knob.h"

using namespace juce;

//==============================================================================
namespace
{
    Theme makeDark()
    {
        Theme t;
        t.background1  = Colour (0xff1a1a1c);  t.background2  = Colour (0xff070708);
        t.panelTop     = Colour (0xff1b1b1d);  t.panelBottom  = Colour (0xff111112);
        t.headerTop    = Colour (0xff2e2e31);  t.headerBottom = Colour (0xff1e1e20);
        t.headerText   = Colour (0xffd9d9db);  t.labelText    = Colour (0xffb4b4b8);
        t.text         = Colour (0xffffffff);
        t.bevelDark    = Colour (0xff000000);  t.bevelLight   = Colour (0x22ffffff);
        t.insetTop     = Colour (0xff050505);  t.insetBottom  = Colour (0xff141415);
        t.lcdTop       = Colour (0xff1c1714);  t.lcdBottom    = Colour (0xff0b0908);
        t.lcdDimTop    = Colour (0xff171718);  t.lcdDimBottom = Colour (0xff0c0c0d);
        t.accent       = Colour (0xffff7a1a);  t.accentGlow   = Colour (0xffe8600c);
        t.accentLight  = Colour (0xffffd2a8);  t.lightGrey    = Colour (0xffc8c8cc);
        t.red          = Colour (0xffff3b30);
        t.buttonTop    = Colour (0xff343437);  t.buttonBottom = Colour (0xff1a1a1c);
        t.buttonText   = Colour (0xffdfe3e6);
        t.barTop       = Colour (0xff222224);  t.barBottom    = Colour (0xff111112);
        t.tickColour   = Colour (0xffc8c8cc).withAlpha (0.6f);
        return t;
    }

    Theme makeLight()
    {
        Theme t = makeDark();
        t.background1  = Colour (0xffd9d9dc);  t.background2  = Colour (0xffb9b9bd);
        t.panelTop     = Colour (0xffe4e4e7);  t.panelBottom  = Colour (0xffcfcfd3);
        t.headerTop    = Colour (0xfff2f2f4);  t.headerBottom = Colour (0xffd6d6da);
        t.headerText   = Colour (0xff2a2a2e);  t.labelText    = Colour (0xff45454a);
        t.text         = Colour (0xff1a1a1c);
        t.bevelDark    = Colour (0xff8a8a90);  t.bevelLight   = Colour (0x66ffffff);
        t.insetTop     = Colour (0xffb2b2b7);  t.insetBottom  = Colour (0xffc9c9ce);
        t.buttonTop    = Colour (0xfff4f4f6);  t.buttonBottom = Colour (0xffcbcbcf);
        t.buttonText   = Colour (0xff26262a);
        t.barTop       = Colour (0xffe9e9ec);  t.barBottom    = Colour (0xffc4c4c8);
        t.tickColour   = Colour (0xff55555a).withAlpha (0.7f);
        return t;
    }

    const Theme darkTheme = makeDark(), lightTheme = makeLight();
    bool lightActive = false;
}

const Theme& Theme::get()           { return lightActive ? lightTheme : darkTheme; }
void Theme::setLight (bool light)   { lightActive = light; }
bool Theme::isLight()               { return lightActive; }

//==============================================================================
AcidLookAndFeel::AcidLookAndFeel()
{
    refreshColours();
}

void AcidLookAndFeel::refreshColours()
{
    const auto& t = Theme::get();
    setColour (ComboBox::textColourId,                   t.text);
    setColour (ComboBox::backgroundColourId,             t.insetBottom);
    setColour (ComboBox::outlineColourId,                t.bevelDark);
    setColour (ComboBox::arrowColourId,                  t.accent);
    setColour (PopupMenu::backgroundColourId,            Theme::isLight() ? Colour (0xffeeeef0) : Colour (0xff121213));
    setColour (PopupMenu::textColourId,                  Theme::isLight() ? Colour (0xff1a1a1c) : Colour (0xffe8ebee));
    setColour (PopupMenu::highlightedBackgroundColourId, Colour (0xffc4520a));
    setColour (PopupMenu::highlightedTextColourId,       Colours::white);
    setColour (PopupMenu::headerTextColourId,            t.labelText);
    setColour (TextButton::textColourOffId,              t.buttonText);
    setColour (TextButton::textColourOnId,               Colours::white);
    setColour (Label::textColourId,                      t.text);
}

Font AcidLookAndFeel::uiFont (float height, bool bold)
{
    return Font (FontOptions (height, bold ? Font::bold : Font::plain));
}

//==============================================================================
void AcidLookAndFeel::drawTicks (Graphics& g, Point<float> c, float rInner, float rOuter,
                                 float start, float end, int count, Colour colour, float thickness)
{
    g.setColour (colour);
    for (int i = 0; i < count; ++i)
    {
        const float a = start + (end - start) * (float) i / (float) (count - 1);
        const float s = std::sin (a), co = -std::cos (a);
        const bool major = (i % 5) == 0;
        const float r0 = major ? rInner - 1.5f : rInner;
        g.drawLine (c.x + s * r0, c.y + co * r0, c.x + s * rOuter, c.y + co * rOuter,
                    major ? thickness * 1.4f : thickness);
    }
}

void AcidLookAndFeel::drawModArc (Graphics& g, Point<float> c, float r, float pos, float start, float end, float depth)
{
    if (std::abs (depth) < 0.001f)
        return;
    const float a0 = start + jlimit (0.0f, 1.0f, pos - std::abs (depth)) * (end - start);
    const float a1 = start + jlimit (0.0f, 1.0f, pos + std::abs (depth)) * (end - start);
    Path arc;
    arc.addCentredArc (c.x, c.y, r, r, 0.0f, a0, a1, true);
    g.setColour (Theme::get().accentLight.withAlpha (0.85f));
    g.strokePath (arc, PathStrokeType (1.6f, PathStrokeType::curved, PathStrokeType::rounded));
}

void AcidLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int width, int height, float pos,
                                        float start, float end, Slider& slider)
{
    auto area = Rectangle<int> (x, y, width, height).toFloat();
    auto* knob = dynamic_cast<AcidKnob*> (&slider);
    const float mod = knob != nullptr ? knob->getModDepth() : 0.0f;

    if (knob != nullptr && knob->getKnobSize() == AcidKnob::Size::Large)
        drawLargeKnob (g, area, pos, start, end, mod);
    else
        drawSmallKnob (g, area, pos, start, end, mod, knob != nullptr && knob->isBipolar());
}

void AcidLookAndFeel::drawLargeKnob (Graphics& g, Rectangle<float> area, float pos, float start, float end, float modDepth)
{
    const auto& t = Theme::get();
    const float size = std::min (area.getWidth(), area.getHeight());
    const auto c = area.getCentre();
    const float R = size * 0.5f - 1.0f;
    const float angle = start + pos * (end - start);

    drawTicks (g, c, R - 4.5f, R, start, end, 41, t.tickColour, 0.8f);

    // orange arc ring: recessed track, glow and bright value arc
    const float ringR = R - 9.5f;
    {
        Path track;
        track.addCentredArc (c.x, c.y, ringR, ringR, 0.0f, start, end, true);
        g.setColour (Colours::black);
        g.strokePath (track, PathStrokeType (7.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (Colour (0xff2a1a10));
        g.strokePath (track, PathStrokeType (4.0f, PathStrokeType::curved, PathStrokeType::rounded));

        if (pos > 0.001f)
        {
            Path value;
            value.addCentredArc (c.x, c.y, ringR, ringR, 0.0f, start, angle, true);
            g.setColour (t.accentGlow.withAlpha (0.18f));
            g.strokePath (value, PathStrokeType (12.0f, PathStrokeType::curved, PathStrokeType::rounded));
            g.setColour (t.accentGlow.withAlpha (0.35f));
            g.strokePath (value, PathStrokeType (7.5f, PathStrokeType::curved, PathStrokeType::rounded));
            g.setColour (t.accent);
            g.strokePath (value, PathStrokeType (4.0f, PathStrokeType::curved, PathStrokeType::rounded));
            g.setColour (t.accentLight.withAlpha (0.8f));
            g.strokePath (value, PathStrokeType (1.2f, PathStrokeType::curved, PathStrokeType::rounded));
        }
        drawModArc (g, c, ringR + 5.5f, pos, start, end, modDepth);
    }

    // chrome dial
    const float dialR = ringR - 7.0f;
    const auto dialBounds = Rectangle<float> (dialR * 2.0f, dialR * 2.0f).withCentre (c);

    Path dial;
    dial.addEllipse (dialBounds);
    DropShadow (Colours::black.withAlpha (0.75f), 10, { 0, 4 }).drawForPath (g, dial);

    g.setGradientFill (ColourGradient (Colour (0xfff5f7f9), dialBounds.getX(), dialBounds.getY(),
                                       Colour (0xff6c737a), dialBounds.getRight(), dialBounds.getBottom(), false));
    g.fillPath (dial);

    const auto face = dialBounds.reduced (dialR * 0.09f);
    ColourGradient faceGrad (Colour (0xffe9ecef), face.getX(), face.getY(),
                             Colour (0xff9ba3ab), face.getRight(), face.getBottom(), false);
    faceGrad.addColour (0.45, Colour (0xffd2d7dc));
    g.setGradientFill (faceGrad);
    g.fillEllipse (face);

    for (float r = face.getWidth() * 0.5f - 1.0f; r > 2.0f; r -= 1.6f)
    {
        const float alpha = 0.035f + 0.03f * std::sin (r * 2.3f);
        g.setColour ((((int) r) % 2 == 0 ? Colours::white : Colours::black).withAlpha (alpha));
        g.drawEllipse (Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c), 0.7f);
    }

    {
        ColourGradient spec (Colours::white.withAlpha (0.55f), face.getX() + face.getWidth() * 0.3f, face.getY() + face.getHeight() * 0.22f,
                             Colours::white.withAlpha (0.0f), face.getCentreX(), face.getCentreY() + face.getHeight() * 0.1f, true);
        g.setGradientFill (spec);
        g.fillEllipse (face);
    }

    g.setColour (Colour (0xff3b4046));
    g.drawEllipse (dialBounds, 1.0f);
    g.setColour (Colours::white.withAlpha (0.6f));
    g.drawEllipse (face, 0.8f);

    const float s = std::sin (angle), co = -std::cos (angle);
    const float p0 = dialR * 0.22f, p1 = dialR * 0.86f;
    g.setColour (Colours::white.withAlpha (0.5f));
    g.drawLine (c.x + s * p0 + 0.8f, c.y + co * p0 + 0.8f, c.x + s * p1 + 0.8f, c.y + co * p1 + 0.8f, 3.2f);
    g.setColour (Colour (0xff1c2025));
    g.drawLine (c.x + s * p0, c.y + co * p0, c.x + s * p1, c.y + co * p1, 3.0f);
}

void AcidLookAndFeel::drawSmallKnob (Graphics& g, Rectangle<float> area, float pos, float start, float end,
                                     float modDepth, bool bipolar)
{
    const auto& t = Theme::get();
    const float size = std::min (area.getWidth(), area.getHeight());
    const auto c = area.getCentre();
    const float R = size * 0.5f - 1.0f;
    const float angle = start + pos * (end - start);

    drawTicks (g, c, R - 3.5f, R, start, end, 21, t.tickColour, 0.8f);

    // thin orange value arc just inside the ticks (from the centre for bipolar knobs)
    {
        const float arcR = R - 5.0f;
        const float from = bipolar ? start + 0.5f * (end - start) : start;
        if (std::abs (angle - from) > 0.01f)
        {
            Path arc;
            arc.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, std::min (from, angle), std::max (from, angle), true);
            g.setColour (t.accent.withAlpha (0.9f));
            g.strokePath (arc, PathStrokeType (1.6f, PathStrokeType::curved, PathStrokeType::rounded));
        }
        drawModArc (g, c, R + 1.5f, pos, start, end, modDepth);
    }

    const float bodyR = R - 7.0f;
    const auto body = Rectangle<float> (bodyR * 2.0f, bodyR * 2.0f).withCentre (c);

    Path bodyPath;
    bodyPath.addEllipse (body);
    DropShadow (Colours::black.withAlpha (0.8f), 6, { 0, 3 }).drawForPath (g, bodyPath);

    g.setGradientFill (ColourGradient (Colour (0xff5a5a5e), body.getX(), body.getY(),
                                       Colour (0xff0a0a0b), body.getX(), body.getBottom(), false));
    g.fillEllipse (body);

    const auto cap = body.reduced (bodyR * 0.14f);
    ColourGradient capGrad (Colour (0xff2c2c2f), cap.getCentreX(), cap.getY(),
                            Colour (0xff0d0d0e), cap.getCentreX(), cap.getBottom(), false);
    g.setGradientFill (capGrad);
    g.fillEllipse (cap);

    g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.12f), cap.getCentreX(), cap.getY(),
                                       Colours::white.withAlpha (0.0f), cap.getCentreX(), cap.getCentreY(), false));
    g.fillEllipse (cap.reduced (1.0f));

    g.setColour (Colours::black.withAlpha (0.8f));
    g.drawEllipse (body, 0.8f);

    const float s = std::sin (angle), co = -std::cos (angle);
    const float p0 = bodyR * 0.35f, p1 = bodyR * 0.92f;
    g.setColour (Colours::white);
    g.drawLine (c.x + s * p0, c.y + co * p0, c.x + s * p1, c.y + co * p1, 1.8f);
}

//==============================================================================
void AcidLookAndFeel::drawInsetBox (Graphics& g, Rectangle<float> b, float corner)
{
    const auto& t = Theme::get();
    g.setColour (t.bevelLight.withAlpha (Theme::isLight() ? 0.6f : 0.12f));
    g.drawRoundedRectangle (b.translated (0.0f, 1.0f), corner, 1.0f);

    g.setGradientFill (ColourGradient (t.insetTop, 0.0f, b.getY(), t.insetBottom, 0.0f, b.getBottom(), false));
    g.fillRoundedRectangle (b, corner);

    g.setColour (Colours::black.withAlpha (Theme::isLight() ? 0.15f : 0.6f));
    g.drawHorizontalLine ((int) b.getY() + 1, b.getX() + corner, b.getRight() - corner);
    g.setColour (t.bevelDark);
    g.drawRoundedRectangle (b.reduced (0.5f), corner, 1.0f);
}

void AcidLookAndFeel::drawComboBox (Graphics& g, int width, int height, bool, int, int, int, int, ComboBox& box)
{
    const auto b = Rectangle<int> (width, height).toFloat();
    drawInsetBox (g, b);

    if (box.isPopupActive())
    {
        g.setColour (Theme::get().accent.withAlpha (0.45f));
        g.drawRoundedRectangle (b.reduced (1.0f), 3.0f, 1.0f);
    }

    const float ax = (float) width - 12.0f, ay = (float) height * 0.5f;
    Path arrow;
    arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (Theme::get().accent);
    g.fillPath (arrow);
}

Font AcidLookAndFeel::getComboBoxFont (ComboBox&)   { return uiFont (12.0f, true); }
Font AcidLookAndFeel::getPopupMenuFont()             { return uiFont (13.0f); }

void AcidLookAndFeel::positionComboBoxText (ComboBox& box, Label& label)
{
    label.setBounds (4, 1, box.getWidth() - 22, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
    label.setJustificationType (Justification::centredLeft);
    label.setColour (Label::textColourId, Theme::get().text);
}

void AcidLookAndFeel::drawPopupMenuBackground (Graphics& g, int width, int height)
{
    g.fillAll (findColour (PopupMenu::backgroundColourId));
    g.setColour (Theme::get().accent.withAlpha (0.35f));
    g.drawRect (0, 0, width, height, 1);
}

//==============================================================================
void AcidLookAndFeel::drawButtonBackground (Graphics& g, Button& button, const Colour&, bool isHighlighted, bool isDown)
{
    const auto& t = Theme::get();
    auto b = button.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = button.getToggleState();

    Colour top = on ? Colour (0xffff8a2a) : t.buttonTop;
    Colour bottom = on ? Colour (0xffc4520a) : t.buttonBottom;
    if (isHighlighted) { top = top.brighter (0.12f); bottom = bottom.brighter (0.12f); }
    if (isDown)        std::swap (top, bottom);

    g.setColour (Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (b.translated (0.0f, 1.0f), 3.0f);
    g.setGradientFill (ColourGradient (top, 0.0f, b.getY(), bottom, 0.0f, b.getBottom(), false));
    g.fillRoundedRectangle (b, 3.0f);
    g.setColour (Colours::white.withAlpha (0.18f));
    g.drawHorizontalLine ((int) b.getY() + 1, b.getX() + 3.0f, b.getRight() - 3.0f);
    g.setColour (t.bevelDark);
    g.drawRoundedRectangle (b, 3.0f, 1.0f);
}

Font AcidLookAndFeel::getTextButtonFont (TextButton&, int)  { return uiFont (11.5f, true); }

void AcidLookAndFeel::drawButtonText (Graphics& g, TextButton& button, bool, bool)
{
    g.setFont (getTextButtonFont (button, button.getHeight()));
    const auto colour = button.getToggleState() ? Colours::white : Theme::get().buttonText;
    g.setColour (colour.withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.4f));
    g.drawFittedText (button.getButtonText(), button.getLocalBounds().reduced (2, 0), Justification::centred, 1, 0.7f);
}

//==============================================================================
void AcidLookAndFeel::drawPanel (Graphics& g, Rectangle<float> b, const String& title)
{
    const auto& t = Theme::get();
    constexpr float corner = 5.0f, headerH = 18.0f;

    g.setColour (t.bevelLight);
    g.drawRoundedRectangle (b.translated (0.5f, 1.0f), corner, 1.0f);

    g.setGradientFill (ColourGradient (t.panelTop, 0.0f, b.getY(), t.panelBottom, 0.0f, b.getBottom(), false));
    g.fillRoundedRectangle (b, corner);

    Path header;
    const auto h = b.withHeight (headerH);
    header.addRoundedRectangle (h.getX(), h.getY(), h.getWidth(), h.getHeight(), corner, corner, true, true, false, false);
    g.setGradientFill (ColourGradient (t.headerTop, 0.0f, h.getY(), t.headerBottom, 0.0f, h.getBottom(), false));
    g.fillPath (header);
    g.setColour (Colours::white.withAlpha (0.25f));
    g.drawHorizontalLine ((int) h.getY() + 1, h.getX() + corner, h.getRight() - corner);
    g.setColour (t.bevelDark);
    g.drawHorizontalLine ((int) h.getBottom(), h.getX(), h.getRight());
    g.setColour (Colours::white.withAlpha (0.07f));
    g.drawHorizontalLine ((int) h.getBottom() + 1, h.getX() + 1.0f, h.getRight() - 1.0f);

    g.setFont (uiFont (10.5f, true));
    if (! Theme::isLight())
    {
        g.setColour (Colours::black.withAlpha (0.45f));
        g.drawText (title.toUpperCase(), h.translated (0.0f, 1.0f), Justification::centred);
    }
    g.setColour (t.headerText);
    g.drawText (title.toUpperCase(), h, Justification::centred);

    g.setColour (t.bevelDark);
    g.drawRoundedRectangle (b, corner, 1.0f);
}

void AcidLookAndFeel::drawLcd (Graphics& g, Rectangle<float> b, bool dimmed)
{
    const auto& t = Theme::get();
    g.setColour (Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (b.expanded (1.0f), 3.0f);

    const auto top = dimmed ? t.lcdDimTop : t.lcdTop;
    const auto bottom = dimmed ? t.lcdDimBottom : t.lcdBottom;
    g.setGradientFill (ColourGradient (top, 0.0f, b.getY(), bottom, 0.0f, b.getBottom(), false));
    g.fillRoundedRectangle (b, 2.5f);

    g.setColour (Colours::white.withAlpha (0.10f));
    g.fillRoundedRectangle (b.withHeight (std::min (b.getHeight() * 0.45f, 14.0f)).reduced (1.0f, 1.0f), 2.0f);
    g.setColour (Colours::black.withAlpha (0.06f));
    for (float yy = b.getY() + 2.0f; yy < b.getBottom() - 1.0f; yy += 2.0f)
        g.drawHorizontalLine ((int) yy, b.getX() + 1.0f, b.getRight() - 1.0f);

    g.setColour (Colours::black.withAlpha (0.7f));
    g.drawRoundedRectangle (b, 2.5f, 1.0f);
}

void AcidLookAndFeel::drawSmallLabel (Graphics& g, const String& text, Rectangle<float> area, Justification j)
{
    g.setFont (uiFont (10.0f, true));
    if (! Theme::isLight())
    {
        g.setColour (Colours::black.withAlpha (0.5f));
        g.drawText (text.toUpperCase(), area.translated (0.0f, 1.0f), j);
    }
    g.setColour (Theme::get().labelText);
    g.drawText (text.toUpperCase(), area, j);
}
