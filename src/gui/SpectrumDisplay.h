#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"
#include "TrackspacerLookAndFeel.h"

namespace ts
{
/** Dark spectrum display: sidechain spectrum in blue, applied reduction in
    white, filter range shading and the little dot that opens the advanced
    panel.
*/
class SpectrumDisplay : public juce::Component,
                        private juce::Timer
{
public:
    explicit SpectrumDisplay (TrackspacerAudioProcessor& processorToUse);

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseMove (const juce::MouseEvent& event) override;
    void mouseExit (const juce::MouseEvent& event) override;

    std::function<void()> onAdvancedPanelToggle;

    void setAdvancedPanelVisible (bool visible) { advancedPanelVisible = visible; repaint(); }

private:
    void timerCallback() override;

    float xForFrequency (float frequencyHz) const;
    float yForSidechainDb (float db) const;
    float yForReductionDb (float db) const;

    void drawGrid (juce::Graphics& g, juce::Rectangle<float> area);
    void drawFilterRange (juce::Graphics& g, juce::Rectangle<float> area, const DisplayFrame& frame);
    void drawCurves (juce::Graphics& g, juce::Rectangle<float> area, const DisplayFrame& frame);
    void drawAdvancedDot (juce::Graphics& g, juce::Rectangle<float> area);
    void drawHoverReadout (juce::Graphics& g, juce::Rectangle<float> area, const DisplayFrame& frame);

    static void buildSmoothPath (juce::Path& path,
                                 const float* xValues,
                                 const float* yValues,
                                 int numPoints);

    TrackspacerAudioProcessor& processor;

    DisplayFrame lastFrame;
    DisplayFrame smoothedFrame;
    bool advancedPanelVisible = false;

    juce::Point<float> hoverPosition;
    bool hovering = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumDisplay)
};
} // namespace ts
