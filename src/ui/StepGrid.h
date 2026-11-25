#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../seq/Step.h"

class AcidAudioProcessor;

/** Piano-roll step editor: one octave (C..C') with note bars, slide connectors and
    octave markers, plus rows for Up, Down, Accent, Slide, Ratchet and Gate. Shows 16
    steps per page; the playhead step is highlighted. */
class StepGrid : public juce::Component
{
public:
    explicit StepGrid (AcidAudioProcessor&);

    void setPage (int p)               { if (p != page) { page = p; repaint(); } }
    int getPage() const noexcept       { return page; }
    void setPlayStep (int step)        { if (step != playStep) { playStep = step; repaint(); } }

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    /** Help text for the cell under the mouse (empty if none). */
    juce::String getHoverHelp() const  { return hoverHelp; }

private:
    enum Row { RowNone = -1, RowNumbers = 0, RowRoll, RowUp, RowDown, RowAccent, RowSlide, RowRatchet, RowGate };

    struct Layout
    {
        juce::Rectangle<float> numbers, roll, rows[6];
        float labelW = 54.0f, colW = 0.0f, gridX = 0.0f, noteH = 0.0f;
    };
    Layout layout() const;
    int columnAt (float x) const;
    Row rowAt (float y, int* noteOut) const;
    void applyEdit (int step, Row row, int note, bool first);

    AcidAudioProcessor& processor;
    int page = 0, playStep = -1;

    // drag state
    Row dragRow = RowNone;
    int lastDragStep = -1;
    bool dragRightButton = false;
    int dragNewValue = 0;
    juce::String hoverHelp;
};
