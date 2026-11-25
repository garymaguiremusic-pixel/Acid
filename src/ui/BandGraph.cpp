#include "BandGraph.h"
#include "LookAndFeel.h"
#include "../PluginProcessor.h"

using namespace juce;
using namespace acid;

namespace
{
    constexpr float kMinHz = 20.0f, kMaxHz = 20000.0f, kDbRange = 18.0f;
    const char* kBandNames[] = { "LOW", "MID", "HIGH" };
}

BandGraph::BandGraph (AcidAudioProcessor& p) : processor (p)
{
    using namespace ParamIDs;
    const char* ids[] = { tubeEdgeLow, tubeXover1, tubeXover2, tubeEdgeHigh };
    for (int i = 0; i < 4; ++i)
        edgeParams[i] = processor.apvts.getParameter (ids[i]);
    for (int b = 0; b < 3; ++b)
        gainParams[b] = processor.apvts.getParameter (band (b, "Gain"));
}

Rectangle<float> BandGraph::plotArea() const
{
    return getLocalBounds().toFloat().reduced (1.0f).withTrimmedLeft (34.0f).withTrimmedRight (18.0f)
                                                    .withTrimmedTop (30.0f).withTrimmedBottom (22.0f);
}

float BandGraph::xForFreq (float hz) const
{
    const auto p = plotArea();
    return p.getX() + p.getWidth() * std::log (jlimit (kMinHz, kMaxHz, hz) / kMinHz) / std::log (kMaxHz / kMinHz);
}

float BandGraph::freqForX (float x) const
{
    const auto p = plotArea();
    return kMinHz * std::pow (kMaxHz / kMinHz, jlimit (0.0f, 1.0f, (x - p.getX()) / p.getWidth()));
}

float BandGraph::yForDb (float db) const
{
    const auto p = plotArea();
    return p.getCentreY() - (db / kDbRange) * p.getHeight() * 0.5f;
}

float BandGraph::dbForY (float y) const
{
    const auto p = plotArea();
    return -(y - p.getCentreY()) / (p.getHeight() * 0.5f) * kDbRange;
}

float BandGraph::edge (int i) const   { return edgeParams[i]->convertFrom0to1 (edgeParams[i]->getValue()); }
float BandGraph::gain (int b) const   { return gainParams[b]->convertFrom0to1 (gainParams[b]->getValue()); }

void BandGraph::paint (Graphics& g)
{
    const auto& t = Theme::get();
    auto b = getLocalBounds().toFloat().reduced (1.0f);
    AcidLookAndFeel::drawLcd (g, b);
    const auto p = plotArea();

    g.setFont (AcidLookAndFeel::uiFont (9.5f, true));
    g.setColour (t.accent.withAlpha (0.75f));
    g.drawText ("DRAG EDGES: FREQUENCY", b.reduced (10.0f, 6.0f), Justification::topLeft);
    g.drawText ("DRAG CENTRES: GAIN", b.reduced (10.0f, 6.0f), Justification::topRight);

    // grid
    g.setFont (AcidLookAndFeel::uiFont (9.0f));
    for (float hz : { 20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f })
    {
        const float x = xForFreq (hz);
        g.setColour (Colours::white.withAlpha (0.07f));
        g.drawVerticalLine ((int) x, p.getY(), p.getBottom());
        g.setColour (Colour (0xffa0a0a4));
        g.drawText (hz >= 1000.0f ? String ((int) (hz / 1000.0f)) + "k" : String ((int) hz),
                    Rectangle<float> (x - 20.0f, p.getBottom() + 4.0f, 40.0f, 12.0f), Justification::centred);
    }
    for (int db = -15; db <= 15; db += 5)
    {
        const float y = yForDb ((float) db);
        g.setColour (Colours::white.withAlpha (db == 0 ? 0.14f : 0.06f));
        g.drawHorizontalLine ((int) y, p.getX(), p.getRight());
        g.setColour (Colour (0xffa0a0a4));
        g.drawText ((db > 0 ? "+" : "") + String (db), Rectangle<float> (b.getX() + 2.0f, y - 6.0f, 28.0f, 12.0f), Justification::centredRight);
    }

    // bands (LR4 band-pass shapes, offset by the band gain)
    const float e[4] = { edge (0), edge (1), edge (2), edge (3) };
    for (int band = 0; band < 3; ++band)
    {
        const float lo = e[band], hi = e[band + 1], gdb = gain (band);
        Path curve;
        bool started = false;
        for (float x = xForFreq (lo); x <= xForFreq (hi) + 0.5f; x += 1.0f)
        {
            const float f = freqForX (x);
            const float rl = std::pow (f / lo, 4.0f), rh = std::pow (f / hi, 4.0f);
            const float mag = (rl / (1.0f + rl)) * (1.0f / (1.0f + rh));
            const float db = jmax (-kDbRange, 20.0f * std::log10 (std::max (1.0e-6f, mag)) + gdb + 3.0f);
            const float y = jmin (p.getBottom(), yForDb (db));
            if (! started) { curve.startNewSubPath (x, p.getBottom()); started = true; }
            curve.lineTo (x, y);
        }
        curve.lineTo (xForFreq (hi), p.getBottom());

        g.setColour (t.accent.withAlpha (0.08f));
        g.fillPath (curve);
        g.setColour (t.accentGlow.withAlpha (0.3f));
        g.strokePath (curve, PathStrokeType (3.0f));
        g.setColour (t.accent);
        g.strokePath (curve, PathStrokeType (1.3f));

        // centre handle
        const float cx = xForFreq (std::sqrt (lo * hi)), cy = yForDb (gdb + 3.0f);
        g.setColour (t.accentLight.withAlpha (0.35f));
        g.fillEllipse (cx - 7.0f, cy - 7.0f, 14.0f, 14.0f);
        g.setColour (t.accent);
        g.fillEllipse (cx - 4.5f, cy - 4.5f, 9.0f, 9.0f);
        g.setFont (AcidLookAndFeel::uiFont (9.0f, true));
        g.drawText (kBandNames[band] + String ("  ") + (gdb > 0 ? "+" : "") + String ((int) gdb) + " dB",
                    Rectangle<float> (cx - 50.0f, cy - 22.0f, 100.0f, 12.0f), Justification::centred);
    }

    // edge handles
    for (int i = 0; i < 4; ++i)
    {
        const float x = xForFreq (e[i]);
        g.setColour (Colour (0xffe0e0e4).withAlpha (0.5f));
        g.drawVerticalLine ((int) x, p.getY() - 6.0f, p.getBottom());
        g.setColour (Colour (0xffe8e8ec));
        g.fillEllipse (x - 4.5f, p.getY() - 10.0f, 9.0f, 9.0f);
    }
}

int BandGraph::findHandle (Point<float> pt, bool& isEdge) const
{
    const auto p = plotArea();
    for (int band = 0; band < 3; ++band)
    {
        const float lo = edge (band), hi = edge (band + 1);
        const Point<float> c (xForFreq (std::sqrt (lo * hi)), yForDb (gain (band) + 3.0f));
        if (c.getDistanceFrom (pt) < 12.0f) { isEdge = false; return band; }
    }
    int best = -1;
    float bestDist = 10.0f;
    for (int i = 0; i < 4; ++i)
    {
        const float d = std::abs (xForFreq (edge (i)) - pt.x);
        if (d < bestDist && pt.y >= p.getY() - 14.0f) { best = i; bestDist = d; }
    }
    isEdge = true;
    return best;
}

void BandGraph::mouseDown (const MouseEvent& e)
{
    dragIndex = findHandle (e.position, dragEdge);
    if (dragIndex < 0) { dragParam = nullptr; return; }
    dragParam = dragEdge ? edgeParams[dragIndex] : gainParams[dragIndex];
    dragParam->beginChangeGesture();
}

void BandGraph::mouseDrag (const MouseEvent& e)
{
    if (dragParam == nullptr)
        return;

    if (dragEdge)
    {
        // keep the edges in order (at least ~1/3 octave apart)
        float hz = freqForX (e.position.x);
        if (dragIndex > 0) hz = jmax (hz, edge (dragIndex - 1) * 1.26f);
        if (dragIndex < 3) hz = jmin (hz, edge (dragIndex + 1) / 1.26f);
        dragParam->setValueNotifyingHost (dragParam->convertTo0to1 (std::round (hz)));
    }
    else
    {
        const float db = jlimit (-15.0f, 15.0f, std::round (dbForY (e.position.y) - 3.0f));
        dragParam->setValueNotifyingHost (dragParam->convertTo0to1 (db));
    }
    repaint();
}

void BandGraph::mouseUp (const MouseEvent&)
{
    if (dragParam != nullptr)
        dragParam->endChangeGesture();
    dragParam = nullptr;
}

void BandGraph::mouseDoubleClick (const MouseEvent& e)
{
    bool isEdge = false;
    const int i = findHandle (e.position, isEdge);
    if (i < 0) return;
    auto* param = isEdge ? edgeParams[i] : gainParams[i];
    param->beginChangeGesture();
    param->setValueNotifyingHost (param->getDefaultValue());
    param->endChangeGesture();
    repaint();
}

void BandGraph::mouseMove (const MouseEvent& e)
{
    bool isEdge = false;
    const int i = findHandle (e.position, isEdge);
    static const char* edgeNames[] = { "LOW EDGE", "CROSSOVER 1", "CROSSOVER 2", "HIGH EDGE" };
    if (i < 0)
        hoverHelp = "BAND GRAPH  -  drag the edges for the frequencies, the centres for each band's gain";
    else if (isEdge)
        hoverHelp = String (edgeNames[i]) + "  " + formatHz (edge (i)) + "  -  drag sideways, double-click = default";
    else
        hoverHelp = String (kBandNames[i]) + " GAIN  " + String ((int) gain (i)) + " dB  -  drag up / down, double-click = 0 dB";
    setMouseCursor (i < 0 ? MouseCursor::NormalCursor : (isEdge ? MouseCursor::LeftRightResizeCursor : MouseCursor::UpDownResizeCursor));
}
