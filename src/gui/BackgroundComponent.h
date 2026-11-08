#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace ts
{
/** Draws the static CarveSpace metal faceplate: rounded plate, brushed metal
    surface, engraved labels, scale markings and the brand badge. The knobs and
    the dark spectrum panel are separate child components drawn on top.
*/
class BackgroundComponent : public juce::Component,
                            private juce::Timer
{
public:
    BackgroundComponent();

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;

    std::function<void()> onLogoClicked;

    /** Clockwise rotation of the black-hole swirl (radians). */
    float logoAngle = 0.0f;
    bool logoPaused = false;

    /** True while the swirl is animating. */
    bool isLogoAnimating() const noexcept { return ! logoPaused; }

    /** Starts/stops the swirl animation (left-clicking the badge toggles it). */
    void setLogoPaused (bool shouldBePaused);

private:
    void timerCallback() override;
    void drawPlate (juce::Graphics& g);
    void drawLabels (juce::Graphics& g);
    void drawLogo (juce::Graphics& g);

    juce::Image grainTexture;
    juce::Image brushedTexture;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BackgroundComponent)
};
} // namespace ts
