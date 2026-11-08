#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace ts
{
/** Rotary slider wired for the Trackspacer-style knobs. */
class ParameterKnob : public juce::Slider
{
public:
    ParameterKnob (const juce::String& componentName, juce::LookAndFeel* lookAndFeel);

    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    std::function<void()> onHoverChanged;
};
} // namespace ts
