#include "ParameterKnob.h"

#include <cmath>

namespace ts
{
namespace
{
    // Total rotary sweep of the knobs: -139 .. +139 degrees.
    constexpr double rotarySweepRadians = 2.0 * 2.426;
    constexpr double twoPi = juce::MathConstants<double>::twoPi;
    constexpr double pi = juce::MathConstants<double>::pi;
} // namespace

ParameterKnob::ParameterKnob (const juce::String& componentName, juce::LookAndFeel* lookAndFeel)
    : juce::Slider (componentName)
{
    setLookAndFeel (lookAndFeel);
    setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    setRotaryParameters (juce::degreesToRadians (-139.0f), juce::degreesToRadians (139.0f), true);
    setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    setPopupDisplayEnabled (false, false, nullptr);
    setDoubleClickReturnValue (true, 0.0);
    setWantsKeyboardFocus (false);
    setMouseDragSensitivity (260);
    setScrollWheelEnabled (true);
}

double ParameterKnob::angleForPoint (juce::Point<float> position, juce::Point<float> centre)
{
    // 0 radians = pointing up, positive = clockwise.
    return std::atan2 ((double) (position.x - centre.x), (double) (centre.y - position.y));
}

void ParameterKnob::mouseDown (const juce::MouseEvent& event)
{
    if (event.mods.isPopupMenu())
    {
        juce::Slider::mouseDown (event);
        return;
    }

    dragging = true;
    lastAngle = angleForPoint (event.position, getLocalBounds().toFloat().getCentre());
    dragProportion = valueToProportionOfLength (getValue());

    // Modifier-free rotary drag: keep the value continuous from its current spot.
    repaint();
}

void ParameterKnob::mouseDrag (const juce::MouseEvent& event)
{
    if (! dragging)
        return;

    const auto centre = getLocalBounds().toFloat().getCentre();
    const double angle = angleForPoint (event.position, centre);

    double delta = angle - lastAngle;

    while (delta > pi)  delta -= twoPi;
    while (delta < -pi) delta += twoPi;

    lastAngle = angle;

    const double fineFactor = event.mods.isShiftDown() ? 0.15 : 1.0;
    dragProportion = juce::jlimit (0.0, 1.0, dragProportion + (delta / rotarySweepRadians) * fineFactor);

    const double newValue = proportionOfLengthToValue (dragProportion);

    if (std::abs (newValue - getValue()) > 1.0e-9)
        setValue (newValue, juce::sendNotificationSync);

    repaint();
}

void ParameterKnob::mouseUp (const juce::MouseEvent& event)
{
    dragging = false;
    juce::ignoreUnused (event);
    repaint();
}

void ParameterKnob::mouseDoubleClick (const juce::MouseEvent& event)
{
    if (isDoubleClickReturnEnabled())
        setValue (getDoubleClickReturnValue(), juce::sendNotificationSync);

    juce::Slider::mouseDoubleClick (event);
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
