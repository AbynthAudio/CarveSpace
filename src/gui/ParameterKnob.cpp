#include "ParameterKnob.h"

namespace ts
{
ParameterKnob::ParameterKnob (const juce::String& componentName, juce::LookAndFeel* lookAndFeel)
    : juce::Slider (componentName)
{
    setLookAndFeel (lookAndFeel);
    setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                         juce::MathConstants<float>::pi * 2.8f,
                         true);
    setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    setPopupDisplayEnabled (false, false, nullptr);
    setDoubleClickReturnValue (true, 0.0);
    setWantsKeyboardFocus (false);
    setMouseDragSensitivity (200);
}

void ParameterKnob::mouseEnter (const juce::MouseEvent&)
{
    if (onHoverChanged != nullptr)
        onHoverChanged();
}

void ParameterKnob::mouseExit (const juce::MouseEvent&)
{
    if (onHoverChanged != nullptr)
        onHoverChanged();
}
} // namespace ts
