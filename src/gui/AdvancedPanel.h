#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"
#include "ParameterKnob.h"
#include "TrackspacerLookAndFeel.h"

namespace ts
{
/** The "Advanced Panel" that slides over the spectrum display. */
class AdvancedPanel : public juce::Component
{
public:
    AdvancedPanel (TrackspacerAudioProcessor& processorToUse, TrackspacerLookAndFeel& lookAndFeelToUse);

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& event) override;

    std::function<void()> onClose;

    /** The M/S mode is drawn as two small radio buttons. */
    class ModeSwitch : public juce::Component
    {
    public:
        explicit ModeSwitch (juce::RangedAudioParameter& parameter);

        void paint (juce::Graphics& g) override;
        void mouseDown (const juce::MouseEvent& event) override;

    private:
        juce::ParameterAttachment attachment;
        float currentValue = 0.0f;
    };

private:
    TrackspacerAudioProcessor& processor;
    TrackspacerLookAndFeel& lookAndFeel;

    ParameterKnob panKnob { "Pan", &lookAndFeel };
    ParameterKnob attackKnob { "Attack", &lookAndFeel };
    ParameterKnob releaseKnob { "Release", &lookAndFeel };
    ModeSwitch modeSwitch;
    juce::ToggleButton scAuditionButton { "Sidechain" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> panAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> scAuditionAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdvancedPanel)
};
} // namespace ts
