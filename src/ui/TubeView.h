#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/** A procedurally drawn vacuum tube whose heater and plates glow with the band's
    drive and level. */
class TubeView : public juce::Component
{
public:
    void setGlow (float drive01, float level);
    void paint (juce::Graphics&) override;

private:
    float drive = 0.3f, level = 0.0f;
};
