#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace ts
{
/** Draws the static Trackspacer-style metal faceplate: rounded plate, labels,
    logo and markings. Knobs and the display are separate child components.
*/
class BackgroundComponent : public juce::Component
{
public:
    BackgroundComponent();

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& event) override;

    std::function<void()> onLogoClicked;

private:
    void drawPlate (juce::Graphics& g);
    void drawLabels (juce::Graphics& g);
    void drawLogo (juce::Graphics& g);

    juce::Image noiseTexture;
};
} // namespace ts
