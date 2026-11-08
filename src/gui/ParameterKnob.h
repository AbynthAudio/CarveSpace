#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace ts
{
/** Rotary slider wired for the CarveSpace-style metal knobs.

    Uses true angle-following rotary dragging: the value follows the angle of
    the mouse around the knob centre (hold Shift for fine adjustment), and every
    change repaints the knob so the rim dot moves smoothly.
*/
class ParameterKnob : public juce::Slider
{
public:
    ParameterKnob (const juce::String& componentName, juce::LookAndFeel* lookAndFeel);

    /** Disc radius as a fraction of half the component width (shadow padding). */
    void setDiscScale (float scale) noexcept { discScale = scale; }
    float getDiscScale() const noexcept { return discScale; }

    void mouseDown (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;
    void mouseDoubleClick (const juce::MouseEvent& event) override;

    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    std::function<void()> onHoverChanged;

private:
    static double angleForPoint (juce::Point<float> position, juce::Point<float> centre);

    float discScale = 1.0f;
    bool dragging = false;
    double lastAngle = 0.0;
    double dragProportion = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterKnob)
};
} // namespace ts
