#include "StepGrid.h"
#include "LookAndFeel.h"
#include "../PluginProcessor.h"

using namespace juce;
using namespace acid;

namespace
{
    const char* kNoteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B", "C'" };
    const char* kRowNames[] = { "UP", "DOWN", "ACCENT", "SLIDE", "RATCHET", "GATE" };
}

StepGrid::StepGrid (AcidAudioProcessor& p) : processor (p) {}

StepGrid::Layout StepGrid::layout() const
{
    Layout l;
    auto b = getLocalBounds().toFloat();
    l.gridX = b.getX() + l.labelW;
    l.colW = (b.getWidth() - l.labelW) / 16.0f;
    l.numbers = b.removeFromTop (14.0f);
    b.removeFromTop (2.0f);
    const float rowH = 17.0f, gap = 1.5f;
    auto rowsArea = b.removeFromBottom (6.0f * (rowH + gap));
    b.removeFromBottom (4.0f);
    l.roll = b;
    l.noteH = l.roll.getHeight() / 13.0f;
    for (int i = 0; i < 6; ++i)
        l.rows[i] = rowsArea.removeFromTop (rowH + gap).withTrimmedBottom (gap);
    return l;
}

int StepGrid::columnAt (float x) const
{
    const auto l = layout();
    if (x < l.gridX) return -1;
    const int c = (int) ((x - l.gridX) / l.colW);
    return c >= 0 && c < 16 ? page * 16 + c : -1;
}

StepGrid::Row StepGrid::rowAt (float y, int* noteOut) const
{
    const auto l = layout();
    if (y < l.numbers.getBottom()) return RowNumbers;
    if (y >= l.roll.getY() && y < l.roll.getBottom())
    {
        if (noteOut) *noteOut = jlimit (0, 12, 12 - (int) ((y - l.roll.getY()) / l.noteH));
        return RowRoll;
    }
    for (int i = 0; i < 6; ++i)
        if (y >= l.rows[i].getY() && y < l.rows[i].getBottom() + 1.5f)
            return (Row) (RowUp + i);
    return RowNone;
}

//==============================================================================
void StepGrid::paint (Graphics& g)
{
    const auto& t = Theme::get();
    const auto l = layout();
    const int slot = processor.getCurrentSlot();
    const auto& pat = processor.getPattern (slot);
    const auto bounds = getLocalBounds().toFloat();

    // backgrounds
    AcidLookAndFeel::drawInsetBox (g, l.roll.withX (l.gridX).withRight (bounds.getRight()), 3.0f);

    // note rows: label column + alternating shading for black keys
    for (int n = 0; n <= 12; ++n)
    {
        const float y = l.roll.getY() + (float) (12 - n) * l.noteH;
        const bool black = n == 1 || n == 3 || n == 6 || n == 8 || n == 10;
        if (black)
        {
            g.setColour (Colours::black.withAlpha (Theme::isLight() ? 0.06f : 0.25f));
            g.fillRect (l.gridX + 1.0f, y, bounds.getRight() - l.gridX - 2.0f, l.noteH);
        }
        g.setFont (AcidLookAndFeel::uiFont (8.5f, true));
        g.setColour (t.labelText.withAlpha (black ? 0.7f : 1.0f));
        g.drawText (kNoteNames[n], Rectangle<float> (bounds.getX(), y, l.labelW - 6.0f, l.noteH), Justification::centredRight);
    }

    // columns: length shading, beat lines, playhead
    for (int c = 0; c < 16; ++c)
    {
        const int step = page * 16 + c;
        const float x = l.gridX + (float) c * l.colW;
        const bool inPattern = step < pat.length;

        if (step == playStep)
        {
            g.setColour (t.accent.withAlpha (0.13f));
            g.fillRect (x, l.roll.getY(), l.colW, l.roll.getHeight());
        }
        if (! inPattern)
        {
            g.setColour (Colours::black.withAlpha (Theme::isLight() ? 0.12f : 0.4f));
            g.fillRect (x, l.roll.getY(), l.colW, l.roll.getHeight());
        }
        if (c > 0)
        {
            g.setColour (c % 4 == 0 ? t.labelText.withAlpha (0.35f) : Colours::white.withAlpha (Theme::isLight() ? 0.25f : 0.05f));
            g.drawVerticalLine ((int) x, l.roll.getY() + 1.0f, l.roll.getBottom() - 1.0f);
        }

        // step number (click to set the length)
        auto numArea = Rectangle<float> (x, l.numbers.getY(), l.colW, l.numbers.getHeight());
        g.setFont (AcidLookAndFeel::uiFont (9.5f, true));
        const bool downbeat = c % 4 == 0;
        g.setColour (step == playStep ? t.accent
                     : (inPattern ? (downbeat ? t.text : t.labelText) : t.labelText.withAlpha (0.4f)));
        g.drawText (String (step + 1), numArea, Justification::centred);
        if (step == pat.length - 1)
        {
            g.setColour (t.accent.withAlpha (0.7f));
            g.fillRect (numArea.getRight() - 2.0f, numArea.getY() + 2.0f, 2.0f, numArea.getHeight() - 4.0f);
        }
    }

    // note bars and slide connectors
    int prevNote = -1;
    float prevBarRight = 0.0f, prevBarY = 0.0f;
    bool prevSlide = false;
    for (int c = 0; c < 16; ++c)
    {
        const int step = page * 16 + c;
        const auto& st = pat.steps[(size_t) step];
        const float x = l.gridX + (float) c * l.colW;
        const bool inPattern = step < pat.length;
        const float alpha = inPattern ? 1.0f : 0.3f;

        int shownNote = st.note;
        if (st.gate == Gate::Tie) shownNote = prevNote;
        if (st.gate == Gate::Rest || shownNote < 0)
        {
            g.setColour (t.labelText.withAlpha (0.35f * alpha));
            g.fillRect (x + l.colW * 0.4f, l.roll.getBottom() - 3.0f, l.colW * 0.2f, 1.5f);
            prevNote = -1;
            prevSlide = false;
            continue;
        }

        const float y = l.roll.getY() + (float) (12 - shownNote) * l.noteH;
        auto bar = Rectangle<float> (x + 3.0f, y + 1.0f, l.colW - 6.0f, l.noteH - 2.0f);
        if (prevSlide || st.gate == Gate::Tie) bar.setX (x);   // joined to the previous step

        Colour fill = st.gate == Gate::Tie ? t.accent.withAlpha (0.55f) : t.accent;
        if (st.accent) fill = t.accentLight;
        g.setColour (t.accentGlow.withAlpha (0.25f * alpha));
        g.fillRoundedRectangle (bar.expanded (1.5f), 2.0f);
        g.setColour (fill.withMultipliedAlpha (alpha));
        g.fillRoundedRectangle (bar, 1.5f);

        // octave marker inside the bar
        String marker;
        if (st.octave > 0) marker = String::repeatedString ("^", st.octave);
        if (st.octave < 0) marker = String::repeatedString ("v", -st.octave);
        if (st.gate == Gate::Note && marker.isNotEmpty())
        {
            g.setColour (Colours::black.withAlpha (0.85f * alpha));
            g.setFont (AcidLookAndFeel::uiFont (9.0f, true));
            g.drawText (marker, bar.reduced (3.0f, 0.0f), Justification::centredLeft);
        }
        if (st.ratchet > 0 && st.gate == Gate::Note)
        {
            g.setColour (Colours::black.withAlpha (0.7f * alpha));
            for (int r = 1; r <= st.ratchet; ++r)
            {
                const float rx = bar.getX() + bar.getWidth() * (float) r / (float) (st.ratchet + 1);
                g.drawVerticalLine ((int) rx, bar.getY(), bar.getBottom());
            }
        }

        // slide connector from the previous bar
        if (prevSlide && prevNote >= 0 && prevNote != shownNote)
        {
            g.setColour (t.accent.withMultipliedAlpha (alpha));
            g.drawLine (prevBarRight, prevBarY + l.noteH * 0.5f, bar.getX() + 2.0f, bar.getY() + l.noteH * 0.5f - 1.0f, 2.0f);
        }

        prevNote = shownNote;
        prevBarRight = bar.getRight();
        prevBarY = y;
        prevSlide = st.slide;
    }

    // option rows
    for (int r = 0; r < 6; ++r)
    {
        const auto row = l.rows[r];
        AcidLookAndFeel::drawSmallLabel (g, kRowNames[r], row.withWidth (l.labelW - 4.0f), Justification::centredLeft);

        for (int c = 0; c < 16; ++c)
        {
            const int step = page * 16 + c;
            const auto& st = pat.steps[(size_t) step];
            const bool inPattern = step < pat.length;
            auto cell = Rectangle<float> (l.gridX + (float) c * l.colW + 2.0f, row.getY(), l.colW - 4.0f, row.getHeight());

            g.setGradientFill (ColourGradient (t.buttonTop, 0.0f, cell.getY(), t.buttonBottom, 0.0f, cell.getBottom(), false));
            g.fillRoundedRectangle (cell, 2.5f);
            g.setColour (t.bevelDark);
            g.drawRoundedRectangle (cell, 2.5f, 1.0f);
            if (! inPattern)
            {
                g.setColour (Colours::black.withAlpha (Theme::isLight() ? 0.15f : 0.45f));
                g.fillRoundedRectangle (cell, 2.5f);
            }

            const float a = inPattern ? 1.0f : 0.35f;
            auto lit = [&] (int count)
            {
                // one or two short orange dashes; an unlit cell shows a dim dash
                for (int k = 0; k < std::max (1, count); ++k)
                {
                    auto dash = cell.withSizeKeepingCentre (cell.getWidth() * 0.42f, 2.5f)
                                    .translated (0.0f, count > 1 ? (k == 0 ? -2.5f : 2.5f) : 0.0f);
                    g.setColour (count > 0 ? t.accent.withMultipliedAlpha (a) : t.labelText.withAlpha (0.25f * a));
                    g.fillRoundedRectangle (dash, 1.0f);
                }
            };

            switch (r)
            {
                case 0: lit (std::max (0, (int) st.octave)); break;
                case 1: lit (std::max (0, -(int) st.octave)); break;
                case 2: lit (st.accent ? 1 : 0); break;
                case 3: lit (st.slide ? 1 : 0); break;
                case 4:
                    g.setFont (AcidLookAndFeel::uiFont (10.0f, true));
                    g.setColour (st.ratchet > 0 ? t.accent.withMultipliedAlpha (a) : t.labelText.withAlpha (0.6f * a));
                    g.drawText (st.ratchet > 0 ? "x" + String (st.ratchet + 1) : String ("1"), cell, Justification::centred);
                    break;
                case 5:
                {
                    const char* txt = st.gate == Gate::Rest ? "REST" : (st.gate == Gate::Tie ? "TIE" : "NOTE");
                    g.setFont (AcidLookAndFeel::uiFont (9.0f, true));
                    g.setColour (st.gate == Gate::Note ? t.accent.withMultipliedAlpha (a) : t.labelText.withAlpha (0.8f * a));
                    g.drawText (txt, cell, Justification::centred);
                    break;
                }
                default: break;
            }
        }
    }
}

//==============================================================================
void StepGrid::applyEdit (int step, Row row, int note, bool first)
{
    if (step < 0 || step >= kMaxSteps)
        return;
    const int slot = processor.getCurrentSlot();
    const auto& current = processor.getPattern (slot).steps[(size_t) step];

    if (first)
    {
        // the value the whole drag paints is decided by the first cell
        switch (row)
        {
            case RowUp:      dragNewValue = current.octave > 0 ? (current.octave >= 2 ? 0 : current.octave + 1) : 1; break;
            case RowDown:    dragNewValue = current.octave < 0 ? (current.octave <= -2 ? 0 : current.octave - 1) : -1; break;
            case RowAccent:  dragNewValue = current.accent ? 0 : 1; break;
            case RowSlide:   dragNewValue = current.slide ? 0 : 1; break;
            case RowRatchet: dragNewValue = (current.ratchet + 1) % (kMaxRatchet + 1); break;
            case RowGate:    dragNewValue = ((int) current.gate + 1) % 3; break;
            case RowRoll:    dragNewValue = dragRightButton ? (current.gate == Gate::Rest ? 0 : 1) : 0; break;
            default: break;
        }
    }

    const int value = dragNewValue;
    const bool rightButton = dragRightButton;
    processor.editPattern (slot, [=] (Pattern& p)
    {
        auto& st = p.steps[(size_t) step];
        switch (row)
        {
            case RowRoll:
                if (rightButton) st.gate = value ? Gate::Rest : Gate::Note;
                else { st.note = (uint8_t) note; st.gate = Gate::Note; }
                break;
            case RowUp:
            case RowDown:    st.octave = (int8_t) value; break;
            case RowAccent:  st.accent = value != 0; break;
            case RowSlide:   st.slide = value != 0; break;
            case RowRatchet: st.ratchet = (uint8_t) value; break;
            case RowGate:    st.gate = (Gate) value; break;
            case RowNumbers: p.length = step + 1; break;
            default: break;
        }
    }, first);   // one undo step per drag
}

void StepGrid::mouseDown (const MouseEvent& e)
{
    int note = 0;
    const int step = columnAt (e.position.x);
    dragRow = rowAt (e.position.y, &note);
    dragRightButton = e.mods.isPopupMenu();
    lastDragStep = step;
    if (step >= 0 && dragRow != RowNone)
        applyEdit (step, dragRow, note, true);
}

void StepGrid::mouseDrag (const MouseEvent& e)
{
    if (dragRow == RowNone || dragRow == RowNumbers)
        return;
    int note = 0;
    const int step = columnAt (e.position.x);
    if (dragRow == RowRoll)
    {
        rowAt (jlimit (layout().roll.getY(), layout().roll.getBottom() - 1.0f, e.position.y), &note);
        if (step >= 0)
        {
            applyEdit (step, dragRow, note, false);
            lastDragStep = step;
        }
        return;
    }
    if (step >= 0 && step != lastDragStep)
    {
        applyEdit (step, dragRow, note, false);
        lastDragStep = step;
    }
}

void StepGrid::mouseUp (const MouseEvent&)
{
    dragRow = RowNone;
}

void StepGrid::mouseMove (const MouseEvent& e)
{
    int note = 0;
    const int step = columnAt (e.position.x);
    const Row row = rowAt (e.position.y, &note);
    String text;
    if (step >= 0)
    {
        const String s = "STEP " + String (step + 1) + "  ";
        switch (row)
        {
            case RowNumbers: text = s + "-  click to end the pattern here"; break;
            case RowRoll:    text = s + kNoteNames[note] + "  -  click / drag to set notes, right-click = rest"; break;
            case RowUp:      text = s + "UP  -  octave up (click again for two)"; break;
            case RowDown:    text = s + "DOWN  -  octave down (click again for two)"; break;
            case RowAccent:  text = s + "ACCENT  -  louder, with a filter kick"; break;
            case RowSlide:   text = s + "SLIDE  -  hold the gate and glide into the next step"; break;
            case RowRatchet: text = s + "RATCHET  -  1 to 4 hits in the step"; break;
            case RowGate:    text = s + "GATE  -  Note / Rest / Tie (extends the previous note)"; break;
            default: break;
        }
    }
    hoverHelp = text;
}

void StepGrid::mouseExit (const MouseEvent&)
{
    hoverHelp = {};
}
