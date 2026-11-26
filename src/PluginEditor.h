#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "ui/LookAndFeel.h"
#include "ui/Knob.h"
#include "ui/Widgets.h"
#include "ui/pages/SynthPage.h"
#include "ui/pages/SequencerPage.h"
#include "ui/pages/TubePage.h"

//==============================================================================
/** The whole interface at its base size (1000 x 900). The editor below scales it
    to the window size. */
class AcidPanel : public juce::Component,
                  private juce::Timer
{
public:
    static constexpr int kBaseWidth = 1000, kBaseHeight = 900;

    explicit AcidPanel (AcidAudioProcessor&);
    ~AcidPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    void showPage (int page);
    void createBackground();
    void drawWordmark (juce::Graphics&, juce::Rectangle<float> area);
    void showPresetMenu();
    void savePreset();
    void applyTheme();
    void trackHover (juce::Component*);
    juce::String helpFor (juce::Component*) const;

    AcidAudioProcessor& processor;
    AcidLookAndFeel lnf;
    juce::Image background;

    // header
    juce::TextButton saveButton { "SAVE" }, clearButton { "CLEAR" };
    juce::TextButton presetPrev { juce::CharPointer_UTF8 ("\xe2\x97\x80") }, presetNext { juce::CharPointer_UTF8 ("\xe2\x96\xb6") };
    HelpDisplay display;
    juce::TextButton echoButton { "Delay" }, tubeButton { "Dist" }, themeButton { "DARK" };
    juce::TextButton randomPatchButton { "RANDOM" }, undoPatchButton { "UNDO" };
    AcidKnob tuneKnob { AcidKnob::Size::Small, "Tune" }, volumeKnob { AcidKnob::Size::Small, "Volume" };

    // pages
    // A (Acid) / B (303) / C (Layer) choose the OSC section on top of the sequencer; Distortion is its own page
    std::array<juce::TextButton, 3> tabs;
    SynthPage synthPage;          // the OSC section + the sequencer
    int currentPage = 0;

    HelpButton helpButton;
    HelpOverlay helpOverlay;

    std::vector<std::unique_ptr<KnobAttachment>> knobAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttonAttachments;

    juce::Component::SafePointer<juce::Component> hovered;
    int lastPresetVersion = -1;
    bool lightShown = false;
    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AcidPanel)
};

//==============================================================================
/** Resizable editor (fixed aspect ratio, 60 % .. 200 %); remembers its size. */
class AcidAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit AcidAudioProcessorEditor (AcidAudioProcessor&);
    void resized() override;

private:
    AcidAudioProcessor& processor;
    AcidPanel panel;
    bool sizeRestored = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AcidAudioProcessorEditor)
};
