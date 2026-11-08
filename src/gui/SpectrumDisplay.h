#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"
#include "CarveSpaceLookAndFeel.h"

namespace ts
{
/** Dark spectrum analyser panel: blue sidechain spectrum, white reduction curve,
    log-frequency grid, low/high-cut shading, hover readout and the little blue
    dot that opens the advanced panel.
*/
class SpectrumDisplay : public juce::Component,
                        private juce::Timer
{
public:
    explicit SpectrumDisplay (CarveSpaceAudioProcessor& processorToUse);

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseMove (const juce::MouseEvent& event) override;
    void mouseExit (const juce::MouseEvent& event) override;

    std::function<void()> onAdvancedPanelToggle;

    void setAdvancedPanelVisible (bool visible)
    {
        advancedPanelVisible = visible;
        repaint();
    }

    static juce::Point<float> advancedDotPosition (juce::Rectangle<float> localBounds);

private:
    void timerCallback() override;

    float xForFrequency (float frequencyHz) const;
    float frequencyForX (float x) const;
    float yForSidechainDb (float db) const;
    float yForReductionDb (float db) const;

    juce::Rectangle<float> plotArea() const;

    void drawGrid (juce::Graphics& g);
    void drawFilterRange (juce::Graphics& g, const DisplayFrame& frame);
    void drawCurves (juce::Graphics& g, const DisplayFrame& frame);
    void drawAdvancedDot (juce::Graphics& g);
    void drawHoverReadout (juce::Graphics& g, const DisplayFrame& frame);

    static void buildSmoothPath (juce::Path& path, const float* xValues, const float* yValues, int numPoints);

    CarveSpaceAudioProcessor& processor;

    DisplayFrame smoothedFrame;
    bool advancedPanelVisible = false;

    juce::Point<float> hoverPosition;
    bool hovering = false;

    juce::Image panelGrain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumDisplay)
};
} // namespace ts
