#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../../arp/PatternModel.h"

//==============================================================================
/**
    The step-sequencer editor: a row of step numbers (with the playing step lit),
    volume bars, velocity bars, transpose values, ratchets, accents and glide buttons.

      Volume:     click or drag across steps to draw (bottom = 0 = silent);
                  right-click (or alt-click) toggles a rest; double-click = full.
      Velocity:   click or drag to draw (used by the Step velocity modes); double-click = 100.
      Transpose:  drag up/down (shift = fine), scroll wheel, double-click = 0.
      Ratchet:    click to cycle 1-4 hits per step (shift-click backwards), drag across
                  to copy, scroll wheel, double-click = 1.
      Accent:     click (or drag across) to accent the step (full velocity, filter opens further).
      Glide:      click (or drag across): the step slides into the next one over the Glide time (303 slide).

    Each mouse gesture is one undo step.
*/
class ArpStepGrid : public juce::Component,
                 public juce::SettableTooltipClient,
                 public juce::FileDragAndDropTarget,
                 private juce::ChangeListener,
                 private juce::Timer
{
public:
    ArpStepGrid (as::arp::PatternModel&, juce::RangedAudioParameter& lengthParam, juce::RangedAudioParameter& endParam);
    ~ArpStepGrid() override;

    void setCurrentStep (int step);

    /** While a file is dragged over the grid: the name shown in the "drop to import" overlay (empty = none). */
    void setDropHint (const juce::String& fileName);

    /** A short message over the grid for a few seconds (e.g. what was imported). */
    void showStatus (const juce::String& message);

    /** Files that can be dropped on the grid, and what to do with one. */
    std::function<bool (const juce::File&)> canImport;
    std::function<void (const juce::File&)> onFileDropped;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void fileDragEnter (const juce::StringArray& files, int, int) override;
    void fileDragExit (const juce::StringArray&) override;
    void filesDropped (const juce::StringArray& files, int, int) override;

    /** Optional: snaps a transpose value edited by hand (used by scale lock). */
    std::function<int (int)> quantiseTranspose;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    /** Help text for the lane under the mouse (Acid's header display). */
    juce::String getHoverHelp() const;

private:
    enum class Lane { None, Header, Volume, Velocity, Transpose, Ratchet, Reverse, Accent, Glide };

    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    struct Layout
    {
        juce::Rectangle<float> labels, header, volume, velocity, transpose, ratchet, accent, glide;
        float gridX = 0.0f, stepWidth = 1.0f;
    };

    Layout getLayout() const;
    int numVisibleSteps() const noexcept      { return length; }
    int stepAt (float x) const;
    Lane laneAt (juce::Point<float>) const;
    juce::Rectangle<float> cell (juce::Rectangle<float> lane, int step) const;
    static int valueFromY (float y, juce::Rectangle<float> lane, int minimum);
    int applyQuantise (int transpose) const;

    as::arp::PatternModel& model;
    as::arp::Pattern pattern;
    juce::ParameterAttachment lengthAttachment, endAttachment;
    int length = 16, endStep = 0, currentStep = -1;

    // gesture state
    Lane dragLane = Lane::None;
    int dragStep = -1, dragStartTranspose = 0;
    float dragStartY = 0.0f;
    bool reversePaintValue = false, accentPaintValue = false, glidePaintValue = false;
    int ratchetPaintValue = 1;
    int hoverStep = -1;
    juce::String dropHint, status;
    void timerCallback() override;
    Lane hoverLane = Lane::None;
};
