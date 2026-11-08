#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"
#include "CarveSpaceLookAndFeel.h"

namespace ts
{
/** The "Advanced Panel" overlay that covers the spectrum display.

    Minimal, blue-on-black design like the original: ring knobs for Pan /
    Attack / Release, an L/R + M/S radio pair, a Sidechain button and a small
    blue dot in the bottom-right corner that closes the panel.
*/
class AdvancedPanel : public juce::Component
{
public:
    AdvancedPanel (CarveSpaceAudioProcessor& processorToUse, CarveSpaceLookAndFeel& lookAndFeelToUse);

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& event) override;

    std::function<void()> onClose;

    /** Thin blue ring knob used by the advanced panel. */
    class RingKnob : public juce::Component
    {
    public:
        explicit RingKnob (juce::RangedAudioParameter& parameter);

        void paint (juce::Graphics& g) override;
        void mouseDown (const juce::MouseEvent& event) override;
        void mouseDrag (const juce::MouseEvent& event) override;
        void mouseUp (const juce::MouseEvent& event) override;
        void mouseMove (const juce::MouseEvent& event) override;
        void mouseExit (const juce::MouseEvent& event) override;

        static juce::Point<float> dotPosition (juce::Rectangle<float> bounds, float normalisedValue);

    private:
        void setValueFromPosition (juce::Point<float> position);

        juce::ParameterAttachment attachment;
        float value = 0.0f;
        bool dragging = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RingKnob)
    };

    /** L/R | M/S radio pair. */
    class ModeSwitch : public juce::Component
    {
    public:
        explicit ModeSwitch (juce::RangedAudioParameter& parameter);

        void paint (juce::Graphics& g) override;
        void mouseDown (const juce::MouseEvent& event) override;
        void mouseMove (const juce::MouseEvent& event) override;
        void mouseExit (const juce::MouseEvent& event) override;

    private:
        juce::ParameterAttachment attachment;
        float value = 0.0f;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModeSwitch)
    };

    /** Small ring button (Sidechain audition). */
    class RingButton : public juce::Button
    {
    public:
        RingButton() : juce::Button ("Sidechain")
        {
            setClickingTogglesState (true);
            setMouseCursor (juce::MouseCursor::PointingHandCursor);
        }

        void paintButton (juce::Graphics& g, bool highlighted, bool down) override;
    };

private:
    CarveSpaceAudioProcessor& processor;
    CarveSpaceLookAndFeel& lookAndFeel;

    RingKnob panKnob;
    RingKnob attackKnob;
    RingKnob releaseKnob;
    ModeSwitch modeSwitch;
    RingButton scAuditionButton;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> scAuditionAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdvancedPanel)
};
} // namespace ts
