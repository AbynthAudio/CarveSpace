#include "SpectrumDisplay.h"

#include <cmath>

namespace ts
{
namespace
{
    constexpr float minDisplayDb = -72.0f;
    constexpr float maxDisplayDb = 6.0f;
    constexpr float minReductionDb = -36.0f;

    float bandLogPosition (float frequencyHz)
    {
        constexpr float low = 20.0f;
        constexpr float high = 20000.0f;
        return std::log (juce::jlimit (low, high, frequencyHz) / low) / std::log (high / low);
    }
}

SpectrumDisplay::SpectrumDisplay (TrackspacerAudioProcessor& processorToUse)
    : processor (processorToUse)
{
    setInterceptsMouseClicks (true, true);
    setMouseCursor (juce::MouseCursor::CrosshairCursor);
    lastFrame.lowCutHz = 20.0f;
    lastFrame.highCutHz = 20000.0f;
    smoothedFrame = lastFrame;
    startTimerHz (30);
}

void SpectrumDisplay::timerCallback()
{
    lastFrame = processor.getDisplayFrame();

    // Smooth the curves a little for a silky display.
    constexpr float smoothing = 0.35f;
    for (int band = 0; band < params::numBands; ++band)
    {
        smoothedFrame.bandFrequenciesHz[band] = lastFrame.bandFrequenciesHz[band];
        smoothedFrame.sidechainDb[band] += (lastFrame.sidechainDb[band] - smoothedFrame.sidechainDb[band]) * smoothing;
        smoothedFrame.reductionDb[band] += (lastFrame.reductionDb[band] - smoothedFrame.reductionDb[band]) * smoothing;
    }
    smoothedFrame.lowCutHz = lastFrame.lowCutHz;
    smoothedFrame.highCutHz = lastFrame.highCutHz;

    repaint();
}

float SpectrumDisplay::xForFrequency (float frequencyHz) const
{
    const auto area = getLocalBounds().toFloat().reduced (14.0f, 10.0f);
    return area.getX() + bandLogPosition (frequencyHz) * area.getWidth();
}

float SpectrumDisplay::yForSidechainDb (float db) const
{
    const auto area = getLocalBounds().toFloat().reduced (14.0f, 10.0f);
    const float normalised = juce::jlimit (0.0f, 1.0f, (db - minDisplayDb) / (maxDisplayDb - minDisplayDb));
    return area.getBottom() - normalised * area.getHeight();
}

float SpectrumDisplay::yForReductionDb (float db) const
{
    const auto area = getLocalBounds().toFloat().reduced (14.0f, 10.0f);
    const float normalised = juce::jlimit (0.0f, 1.0f, (db - minReductionDb) / (0.0f - minReductionDb));
    return area.getBottom() - normalised * area.getHeight() * 0.62f;
}

void SpectrumDisplay::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto area = bounds.reduced (14.0f, 10.0f);
    const float corner = 9.0f;

    g.setColour (colours::displayBackground);
    g.fillRoundedRectangle (bounds, corner);

    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), corner, 1.0f);

    drawGrid (g, area);
    drawFilterRange (g, area, smoothedFrame);
    drawCurves (g, area, smoothedFrame);
    drawHoverReadout (g, area, smoothedFrame);

    if (! advancedPanelVisible)
        drawAdvancedDot (g, area);
}

void SpectrumDisplay::drawGrid (juce::Graphics& g, juce::Rectangle<float> area)
{
    const float frequencies[] = { 20.0f, 30.0f, 50.0f, 100.0f, 200.0f, 300.0f, 500.0f,
                                  1000.0f, 2000.0f, 3000.0f, 5000.0f, 10000.0f, 20000.0f };
    const float dbs[] = { 0.0f, -12.0f, -24.0f, -36.0f, -48.0f, -60.0f };

    g.setColour (colours::displayGrid);

    for (float frequency : frequencies)
    {
        const float x = xForFrequency (frequency);
        g.drawVerticalLine (juce::roundToInt (x), area.getY(), area.getBottom());
    }

    for (float db : dbs)
    {
        const float y = yForSidechainDb (db);
        g.drawHorizontalLine (juce::roundToInt (y), area.getX(), area.getRight());
    }

    g.setColour (colours::displayGridBright);
    g.drawHorizontalLine (juce::roundToInt (area.getY() + area.getHeight() * 0.5f), area.getX(), area.getRight());
}

void SpectrumDisplay::drawFilterRange (juce::Graphics& g, juce::Rectangle<float> area, const DisplayFrame& frame)
{
    const float lowX = xForFrequency (frame.lowCutHz);
    const float highX = xForFrequency (frame.highCutHz);

    g.setColour (juce::Colours::black.withAlpha (0.28f));
    if (lowX > area.getX())
        g.fillRect (juce::Rectangle<float>::leftTopRightBottom (area.getX(), area.getY(), lowX, area.getBottom()));
    if (highX < area.getRight())
        g.fillRect (juce::Rectangle<float>::leftTopRightBottom (highX, area.getY(), area.getRight(), area.getBottom()));

    g.setColour (juce::Colours::white.withAlpha (0.14f));
    g.drawVerticalLine (juce::roundToInt (lowX), area.getY(), area.getBottom());
    g.drawVerticalLine (juce::roundToInt (highX), area.getY(), area.getBottom());
}

void SpectrumDisplay::buildSmoothPath (juce::Path& path,
                                       const float* xValues,
                                       const float* yValues,
                                       int numPoints)
{
    if (numPoints < 2)
        return;

    path.startNewSubPath (xValues[0], yValues[0]);

    for (int i = 1; i < numPoints; ++i)
    {
        const float previousX = xValues[i - 1];
        const float previousY = yValues[i - 1];
        const float x = xValues[i];
        const float y = yValues[i];
        const float midX = (previousX + x) * 0.5f;
        const float midY = (previousY + y) * 0.5f;
        path.quadraticTo (previousX, previousY, midX, midY);
    }

    path.lineTo (xValues[numPoints - 1], yValues[numPoints - 1]);
}

void SpectrumDisplay::drawCurves (juce::Graphics& g, juce::Rectangle<float> area, const DisplayFrame& frame)
{
    float xs[params::numBands];
    float sidechainYs[params::numBands];
    float reductionYs[params::numBands];

    for (int band = 0; band < params::numBands; ++band)
    {
        const float frequency = frame.bandFrequenciesHz[band] > 0.0f
                                    ? frame.bandFrequenciesHz[band]
                                    : BandAnalyzer::bandFrequency (band);
        xs[band] = xForFrequency (frequency);
        sidechainYs[band] = yForSidechainDb (frame.sidechainDb[band]);
        reductionYs[band] = yForReductionDb (frame.reductionDb[band]);
    }

    juce::Path sidechainPath;
    buildSmoothPath (sidechainPath, xs, sidechainYs, params::numBands);

    juce::Path reductionPath;
    buildSmoothPath (reductionPath, xs, reductionYs, params::numBands);

    // Subtle glow for the blue sidechain trace.
    g.setColour (colours::sidechainBlue.withAlpha (0.20f));
    g.strokePath (sidechainPath, juce::PathStrokeType (3.2f));

    g.setColour (colours::sidechainBlue);
    g.strokePath (sidechainPath, juce::PathStrokeType (1.7f));

    g.setColour (colours::reductionWhite.withAlpha (0.85f));
    g.strokePath (reductionPath, juce::PathStrokeType (1.5f));

    juce::ignoreUnused (area);
}

void SpectrumDisplay::drawAdvancedDot (juce::Graphics& g, juce::Rectangle<float> area)
{
    const juce::Point<float> dot (area.getRight() - 8.0f, area.getBottom() - 7.0f);
    g.setColour (colours::sidechainBlue.withAlpha (0.30f));
    g.fillEllipse (juce::Rectangle<float> (10.0f, 10.0f).withCentre (dot));
    g.setColour (colours::sidechainBlue);
    g.fillEllipse (juce::Rectangle<float> (5.5f, 5.5f).withCentre (dot));
}

void SpectrumDisplay::drawHoverReadout (juce::Graphics& g, juce::Rectangle<float> area, const DisplayFrame& frame)
{
    if (! hovering || advancedPanelVisible)
        return;

    const float normalisedX = juce::jlimit (0.0f, 1.0f, (hoverPosition.x - area.getX()) / juce::jmax (1.0f, area.getWidth()));
    const float frequency = 20.0f * std::pow (1000.0f, normalisedX);

    // Interpolate the displayed band data at the hover frequency.
    float sidechainDb = -100.0f;

    for (int band = 0; band < params::numBands - 1; ++band)
    {
        const float f0 = frame.bandFrequenciesHz[band];
        const float f1 = frame.bandFrequenciesHz[band + 1];

        if (frequency >= f0 && frequency <= f1)
        {
            const float t = (frequency - f0) / juce::jmax (1.0f, f1 - f0);
            sidechainDb = frame.sidechainDb[band] + (frame.sidechainDb[band + 1] - frame.sidechainDb[band]) * t;
            break;
        }
    }

    const float x = juce::jlimit (area.getX(), area.getRight(), hoverPosition.x);
    g.setColour (juce::Colours::white.withAlpha (0.15f));
    g.drawVerticalLine (juce::roundToInt (x), area.getY(), area.getBottom());

    juce::String text;
    if (frequency >= 1000.0f)
        text << juce::String (frequency / 1000.0f, frequency >= 10000.0f ? 1 : 2) << " kHz";
    else
        text << juce::String (juce::roundToInt (frequency)) << " Hz";

    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (juce::Font::getDefaultSansSerifFontName(), 10.0f, juce::Font::plain));
    g.drawText (text, juce::Rectangle<float> (area.getX() + 6.0f, area.getY() + 4.0f, 90.0f, 13.0f), juce::Justification::left);

    juce::ignoreUnused (sidechainDb);
}

void SpectrumDisplay::mouseDown (const juce::MouseEvent& event)
{
    if (advancedPanelVisible)
        return;

    const auto area = getLocalBounds().toFloat().reduced (14.0f, 10.0f);
    const juce::Point<float> dot (area.getRight() - 8.0f, area.getBottom() - 7.0f);

    if (event.position.getDistanceFrom (dot) < 14.0f)
        if (onAdvancedPanelToggle != nullptr)
            onAdvancedPanelToggle();
}

void SpectrumDisplay::mouseMove (const juce::MouseEvent& event)
{
    hoverPosition = event.position;
    hovering = getLocalBounds().toFloat().contains (event.position);
    repaint();
}

void SpectrumDisplay::mouseExit (const juce::MouseEvent&)
{
    hovering = false;
    repaint();
}
} // namespace ts
