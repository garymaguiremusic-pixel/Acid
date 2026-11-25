#include "TubeView.h"
#include "LookAndFeel.h"

using namespace juce;

void TubeView::setGlow (float d, float l)
{
    if (std::abs (d - drive) > 0.005f || std::abs (l - level) > 0.01f)
    {
        drive = d;
        level = l;
        repaint();
    }
}

void TubeView::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (2.0f);

    // dark window the tube sits in
    g.setColour (Colours::black);
    g.fillRoundedRectangle (b, 10.0f);
    g.setColour (Colour (0xff3a3a3e));
    g.drawRoundedRectangle (b.reduced (1.0f), 10.0f, 2.0f);
    g.setColour (Colours::black.withAlpha (0.8f));
    g.drawRoundedRectangle (b.reduced (3.0f), 9.0f, 1.0f);

    const float glow = jlimit (0.0f, 1.0f, 0.15f + 0.45f * drive + 0.9f * std::sqrt (level) * (0.4f + drive));
    const auto c = b.getCentre();
    const float w = b.getWidth() * 0.5f, h = b.getHeight() * 0.82f;
    auto glass = Rectangle<float> (w, h).withCentre ({ c.x, c.y + b.getHeight() * 0.02f });

    // warm glow behind the glass
    ColourGradient halo (Colour (0xffff7a1a).withAlpha (0.35f * glow), c.x, c.y + 10.0f,
                         Colours::transparentBlack, c.x + w, c.y + 10.0f, true);
    g.setGradientFill (halo);
    g.fillEllipse (glass.expanded (w * 0.4f, h * 0.1f));

    // base and pins
    auto base = Rectangle<float> (w * 0.9f, h * 0.1f).withCentre ({ c.x, glass.getBottom() - h * 0.02f });
    g.setGradientFill (ColourGradient (Colour (0xff2a2a2c), base.getX(), base.getY(), Colour (0xff0c0c0d), base.getX(), base.getBottom(), false));
    g.fillRoundedRectangle (base, 3.0f);

    // glass envelope
    Path env;
    env.addRoundedRectangle (glass.getX(), glass.getY() + h * 0.08f, glass.getWidth(), glass.getHeight() * 0.88f, w * 0.45f);
    env.addEllipse (glass.getCentreX() - w * 0.08f, glass.getY(), w * 0.16f, h * 0.14f);   // the tip
    g.setColour (Colour (0x30c8d0d8));
    g.fillPath (env);

    // plates (anode) and internal structure
    auto plates = Rectangle<float> (w * 0.6f, h * 0.48f).withCentre ({ c.x, c.y + h * 0.02f });
    g.setGradientFill (ColourGradient (Colour (0xff3d3d40), plates.getX(), plates.getY(),
                                       Colour (0xff1c1c1e), plates.getRight(), plates.getY(), false));
    g.fillRect (plates.reduced (plates.getWidth() * 0.02f, 0.0f));
    g.setColour (Colour (0xff5a5a5e).withAlpha (0.8f));
    for (int i = 1; i < 6; ++i)
        g.drawHorizontalLine ((int) (plates.getY() + plates.getHeight() * (float) i / 6.0f), plates.getX(), plates.getRight());
    g.setColour (Colour (0xff6a6a70));
    g.drawRect (plates, 1.0f);

    // glowing heater filaments
    auto heater = plates.reduced (plates.getWidth() * 0.32f, plates.getHeight() * 0.06f);
    const auto hot = Colour (0xffff8a2a).interpolatedWith (Colour (0xffffe0a0), glow * 0.6f);
    g.setColour (hot.withAlpha (0.25f + 0.75f * glow));
    for (int i = 0; i < 2; ++i)
    {
        const float x = heater.getX() + heater.getWidth() * (0.25f + 0.5f * (float) i);
        g.fillRoundedRectangle (x - 1.5f, heater.getY(), 3.0f, heater.getHeight(), 1.5f);
    }
    ColourGradient core (hot.withAlpha (0.55f * glow), c.x, heater.getCentreY(),
                         Colours::transparentBlack, c.x + heater.getWidth() * 1.4f, heater.getCentreY(), true);
    g.setGradientFill (core);
    g.fillEllipse (heater.expanded (heater.getWidth() * 0.9f, heater.getHeight() * 0.2f));

    // glass highlight
    g.setColour (Colours::white.withAlpha (0.12f));
    g.fillRoundedRectangle (glass.getX() + w * 0.12f, glass.getY() + h * 0.16f, w * 0.08f, h * 0.6f, w * 0.04f);
    g.setColour (Colours::white.withAlpha (0.18f));
    g.strokePath (env, PathStrokeType (1.0f));
}
