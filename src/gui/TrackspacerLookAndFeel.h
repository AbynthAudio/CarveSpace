#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace ts
{
namespace colours
{
    inline const juce::Colour backgroundTop    { 0xffe9e7e3 };
    inline const juce::Colour backgroundBottom { 0xffc8c6c2 };
    inline const juce::Colour border            { 0xff232323 };
    inline const juce::Colour displayBackground { 0xff141516 };
    inline const juce::Colour displayGrid       { 0xff2b2d30 };
    inline const juce::Colour displayGridBright { 0xff383b3f };
    inline const juce::Colour sidechainBlue     { 0xff4f8df7 };
    inline const juce::Colour reductionWhite    { 0xffe9e9e9 };
    inline const juce::Colour label             { 0xff3c3c3e };
    inline const juce::Colour labelBright       { 0xfff2f2f2 };
    inline const juce::Colour knobDark          { 0xff1f1f21 };
    inline const juce::Colour knobMid           { 0xff3a3a3d };
    inline const juce::Colour knobLight         { 0xffd9d9dc };
    inline const juce::Colour accent            { 0xff4f8df7 };
}

class TrackspacerLookAndFeel : public juce::LookAndFeel_V4
{
public:
    TrackspacerLookAndFeel();

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override;

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;

    juce::Font getLabelFont (juce::Label&) override;
};
} // namespace ts
