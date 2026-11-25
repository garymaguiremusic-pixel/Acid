#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../Knob.h"
#include "../Widgets.h"
#include "../LookAndFeel.h"
#include "../../PluginProcessor.h"

/** Shared plumbing for the three pages: parameter attachments and help text. */
class PageBase : public juce::Component
{
public:
    explicit PageBase (AcidAudioProcessor& p) : processor (p) { setInterceptsMouseClicks (false, true); }

    /** Called by the editor's timer (30 Hz). */
    virtual void update() {}

protected:
    void attach (AcidKnob& knob, const juce::String& id)
    {
        knobs.push_back (std::make_unique<KnobAttachment> (*processor.apvts.getParameter (id), knob));
        addAndMakeVisible (knob);
    }

    void attach (juce::TextButton& button, const juce::String& id)
    {
        button.setClickingTogglesState (true);
        buttons.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, id, button));
        setHelp (button, button.getButtonText().toUpperCase() + "  -  " + acid::helpForParameter (id));
        addAndMakeVisible (button);
    }

    void attach (juce::ComboBox& box, const juce::String& id)
    {
        if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (processor.apvts.getParameter (id)))
            box.addItemList (choice->choices, 1);
        combos.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, id, box));
        setHelp (box, processor.apvts.getParameter (id)->getName (64).toUpperCase() + "  -  " + acid::helpForParameter (id));
        addAndMakeVisible (box);
    }

    float value (const juce::String& id) const { return processor.getParameterValue (id); }

    AcidAudioProcessor& processor;

private:
    std::vector<std::unique_ptr<KnobAttachment>> knobs;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttons;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> combos;
};
