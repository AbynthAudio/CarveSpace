#include "SpectrumDisplay.h"

#include <cmath>

namespace ts
{
namespace
{
    constexpr float minDisplayDb = -72.0f;
    constexpr float maxDisplayDb = 6.0f;
    constexpr float minReductionDb = -36.0f;
    constexpr float minFrequency = 20.0f;
    constexpr float maxFrequency = 20000.0f;

    const float gridFrequencies[] =
    {
        20.0f, 30.0f, 40.0f, 50.0f, 60.0f, 70.0f, 80.0f, 90.0f,
        100.0f, 200.0f, 300.0f, 400.0f, 500.0f, 600.0f, 700.0f, 800.0f, 900.0f,
        1000.0f, 1500.0f, 2000.0f, 2500.0f, 3000.0f, 3500.0f, 4000.0f, 4500.0f,
        5000.0f, 5500.0f, 6000.0f, 7000.0f, 8000.0f, 9000.0f, 10000.0f, 15000.0f, 20000.0f
    };

    bool isDecade (float frequency)
    {
        return juce::approximatelyEqual (frequency, 100.0f)
            || juce::approximatelyEqual (frequency, 1000.0f)
            || juce::approximatelyEqual (frequency, 10000.0f);
    }

    float bandFrequency (int band)
    {
        const float ratio = std::pow (maxFrequency / minFrequency, 1.0f / (float) (params::numBands - 1));
        return minFrequency * std::pow (ratio, (float) band);
    }
} // namespace

//==============================================================================
SpectrumDisplay::SpectrumDisplay (CarveSpaceAudioProcessor& processorToUse)
    : processor (processorToUse)
{
    setInterceptsMouseClicks (true, true);
    setMouseCursor (juce::MouseCursor::CrosshairCursor);

    smoothedFrame.lowCutHz = params::cutMin;
    smoothedFrame.highCutHz = params::cutMax;

    // Fine noise for the panel surface.
    panelGrain = juce::Image (juce::Image::ARGB, 256, 256, true);
    juce::Random random (0x1a2b3c4d);

    for (int y = 0; y < panelGrain.getHeight(); ++y)
    {
        for (int x = 0; x < panelGrain.getWidth(); ++x)
        {
            const float v = random.nextFloat();
            const auto alpha = (juce::uint8) juce::jlimit (0, 255, (int) (v * v * 26.0f));
            panelGrain.setPixelAt (x, y, juce::Colour ((juce::uint8) 24, (juce::uint8) 24, (juce::uint8) 24, alpha));
        }
    }

    startTimerHz (30);
}

void SpectrumDisplay::timerCallback()
{
    const auto frame = processor.getDisplayFrame();

    // Gentle smoothing for a silky display.
    constexpr float smoothing = 0.35f;

    for (int band = 0; band < params::numBands; ++band)
    {
        smoothedFrame.bandFrequenciesHz[band] = frame.bandFrequenciesHz[band];
        smoothedFrame.sidechainDb[band] += (frame.sidechainDb[band] - smoothedFrame.sidechainDb[band]) * smoothing;
        smoothedFrame.reductionDb[band] += (frame.reductionDb[band] - smoothedFrame.reductionDb[band]) * smoothing;
    }

    smoothedFrame.lowCutHz = frame.lowCutHz > 0.0f ? frame.lowCutHz : params::cutMin;
    smoothedFrame.highCutHz = frame.highCutHz > 0.0f ? frame.highCutHz : params::cutMax;
    smoothedFrame.hoverFrequencyHz = frame.hoverFrequencyHz;
    smoothedFrame.hoverDb = frame.hoverDb;

    repaint();
}

juce::Rectangle<float> SpectrumDisplay::plotArea() const
{
    const auto bounds = getLocalBounds().toFloat();
    return { bounds.getX() + 4.5f, bounds.getY() + 3.5f,
             bounds.getWidth() - 8.0f, bounds.getHeight() - 7.0f };
}

float SpectrumDisplay::xForFrequency (float frequencyHz) const
{
    const auto area = plotArea();
    const float normalised = std::log (juce::jlimit (minFrequency, maxFrequency, frequencyHz) / minFrequency)
                           / std::log (maxFrequency / minFrequency);
    return area.getX() + normalised * area.getWidth();
}

float SpectrumDisplay::frequencyForX (float x) const
{
    const auto area = plotArea();
    const float normalised = juce::jlimit (0.0f, 1.0f, (x - area.getX()) / juce::jmax (1.0f, area.getWidth()));
    return minFrequency * std::pow (maxFrequency / minFrequency, normalised);
}

float SpectrumDisplay::yForSidechainDb (float db) const
{
    const auto area = plotArea();
    const float normalised = juce::jlimit (0.0f, 1.0f, (db - minDisplayDb) / (maxDisplayDb - minDisplayDb));
    return area.getBottom() - normalised * area.getHeight();
}

float SpectrumDisplay::yForReductionDb (float db) const
{
    const auto area = plotArea();
    const float depth = juce::jlimit (0.0f, 1.0f, db / minReductionDb);
    return area.getY() + area.getHeight() * (0.70f + 0.26f * depth);
}

void SpectrumDisplay::paint (juce::Graphics& g)
{
    const auto& pal = theme::palette();
    const auto bounds = getLocalBounds().toFloat();
    const float corner = layout::displayCorner + pal.displayCornerExtra;

    juce::Path panel;
    panel.addRoundedRectangle (bounds, corner);

    // Panel body with a subtle top-to-bottom falloff.
    {
        juce::ColourGradient body (pal.displayHi, bounds.getCentreX(), bounds.getY(),
                                   pal.displayLo, bounds.getCentreX(), bounds.getBottom(), false);
        body.addColour (0.45, pal.displayHi.interpolatedWith (pal.displayLo, 0.55f));
        g.setGradientFill (body);
        g.fillPath (panel);
    }

    {
        g.saveState();
        g.reduceClipRegion (panel);

        if (panelGrain.isValid())
        {
            g.setTiledImageFill (panelGrain, 0, 0, 0.85f);
            g.fillRect (bounds);
        }

        // Slight glass highlight along the top edge.
        {
            juce::ColourGradient gloss (juce::Colours::white.withAlpha (0.045f), bounds.getCentreX(), bounds.getY(),
                                        juce::Colours::transparentWhite, bounds.getCentreX(), bounds.getY() + 26.0f, false);
            g.setGradientFill (gloss);
            g.fillRect (bounds.withHeight (26.0f));
        }

        drawGrid (g);
        drawFilterRange (g, smoothedFrame);
        drawCurves (g, smoothedFrame);
        drawHoverReadout (g, smoothedFrame);

        g.restoreState();
    }

    // Dark card outline.
    g.setColour (pal.panelOutline);
    g.strokePath (panel, juce::PathStrokeType (1.1f));

    if (! advancedPanelVisible)
        drawAdvancedDot (g);
}

void SpectrumDisplay::drawGrid (juce::Graphics& g)
{
    const auto& pal = theme::palette();
    const auto area = plotArea();

    for (float frequency : gridFrequencies)
    {
        const float x = xForFrequency (frequency);

        if (x < area.getX() - 0.5f || x > area.getRight() + 0.5f)
            continue;

        g.setColour (isDecade (frequency) ? pal.gridLine : pal.gridFaint);
        g.drawVerticalLine (juce::roundToInt (x), area.getY(), area.getBottom());
    }

    // Horizontal dB lines.
    for (int i = 1; i < 8; ++i)
    {
        const float y = area.getY() + area.getHeight() * (float) i / 8.0f;
        g.setColour (pal.gridFaint);
        g.drawHorizontalLine (juce::roundToInt (y), area.getX(), area.getRight());
    }
}

void SpectrumDisplay::drawFilterRange (juce::Graphics& g, const DisplayFrame& frame)
{
    const auto& pal = theme::palette();
    const auto area = plotArea();
    const float lowX = juce::jlimit (area.getX(), area.getRight(), xForFrequency (frame.lowCutHz));
    const float highX = juce::jlimit (area.getX(), area.getRight(), xForFrequency (frame.highCutHz));

    g.setColour (pal.shadow.withAlpha (0.38f));

    if (lowX > area.getX())
        g.fillRect (juce::Rectangle<float>::leftTopRightBottom (area.getX(), area.getY(), lowX, area.getBottom()));

    if (highX < area.getRight())
        g.fillRect (juce::Rectangle<float>::leftTopRightBottom (highX, area.getY(), area.getRight(), area.getBottom()));

    g.setColour (juce::Colours::white.withAlpha (0.13f));
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

void SpectrumDisplay::drawCurves (juce::Graphics& g, const DisplayFrame& frame)
{
    const auto& pal = theme::palette();

    float xs[params::numBands];
    float sidechainYs[params::numBands];
    float reductionYs[params::numBands];

    for (int band = 0; band < params::numBands; ++band)
    {
        const float frequency = frame.bandFrequenciesHz[band] > 0.0f
                                    ? frame.bandFrequenciesHz[band]
                                    : bandFrequency (band);

        xs[band] = xForFrequency (frequency);
        sidechainYs[band] = yForSidechainDb (frame.sidechainDb[band]);
        reductionYs[band] = yForReductionDb (frame.reductionDb[band]);
    }

    juce::Path sidechainPath;
    buildSmoothPath (sidechainPath, xs, sidechainYs, params::numBands);

    juce::Path reductionPath;
    buildSmoothPath (reductionPath, xs, reductionYs, params::numBands);

    // Blue sidechain trace with a soft glow.
    g.setColour (pal.sidechainBlue.withAlpha (0.16f));
    g.strokePath (sidechainPath, juce::PathStrokeType (4.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour (pal.sidechainBlue);
    g.strokePath (sidechainPath, juce::PathStrokeType (1.9f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // White reduction curve, legible over the grid.
    g.setColour (pal.shadow.withAlpha (0.35f));
    g.strokePath (reductionPath, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour (pal.reductionWhite);
    g.strokePath (reductionPath, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

juce::Point<float> SpectrumDisplay::advancedDotPosition (juce::Rectangle<float> localBounds)
{
    return { localBounds.getRight() - 16.6f, localBounds.getBottom() - 17.9f };
}

void SpectrumDisplay::drawAdvancedDot (juce::Graphics& g)
{
    const auto& pal = theme::palette();
    const auto dot = advancedDotPosition (getLocalBounds().toFloat());
    const float radius = 5.6f;

    g.setColour (pal.accent.withAlpha (0.16f));
    g.fillEllipse (juce::Rectangle<float> (radius * 3.2f, radius * 3.2f).withCentre (dot));

    g.setColour (pal.accent);
    g.drawEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (dot), 1.7f);
}

void SpectrumDisplay::drawHoverReadout (juce::Graphics& g, const DisplayFrame& frame)
{
    if (! hovering || advancedPanelVisible)
        return;

    const auto area = plotArea();
    const float x = juce::jlimit (area.getX(), area.getRight(), hoverPosition.x);

    if (hoverPosition.y < area.getY() || hoverPosition.y > area.getBottom())
        return;

    const float frequency = frequencyForX (x);

    // Interpolate the displayed band data at the hovered frequency.
    float sidechainDb = -100.0f;
    float reductionDb = 0.0f;

    for (int band = 0; band < params::numBands - 1; ++band)
    {
        const float f0 = frame.bandFrequenciesHz[band];
        const float f1 = frame.bandFrequenciesHz[band + 1];

        if (frequency >= f0 && frequency <= f1)
        {
            const float t = (frequency - f0) / juce::jmax (1.0f, f1 - f0);
            sidechainDb = frame.sidechainDb[band] + (frame.sidechainDb[band + 1] - frame.sidechainDb[band]) * t;
            reductionDb = frame.reductionDb[band] + (frame.reductionDb[band + 1] - frame.reductionDb[band]) * t;
            break;
        }
    }

    g.setColour (juce::Colours::white.withAlpha (0.16f));
    g.drawVerticalLine (juce::roundToInt (x), area.getY(), area.getBottom());

    juce::String frequencyText;

    if (frequency >= 1000.0f)
        frequencyText << juce::String (frequency / 1000.0f, frequency >= 10000.0f ? 1 : 2) << " kHz";
    else
        frequencyText << juce::String (juce::roundToInt (frequency)) << " Hz";

    juce::String text;
    text << frequencyText << "    " << juce::String (sidechainDb, 1) << " dB";

    if (reductionDb < -0.1f)
        text << "    -" << juce::String (juce::jmax (0.0f, -reductionDb), 1) << " dB";

    g.setColour (juce::Colours::white.withAlpha (0.90f));
    g.setFont (juce::Font (juce::Font::getDefaultSansSerifFontName(), 10.5f, juce::Font::plain));
    g.drawText (text, juce::Rectangle<float> (area.getX() + 4.0f, area.getY() + 3.0f, 280.0f, 14.0f),
                juce::Justification::left);
}

void SpectrumDisplay::mouseDown (const juce::MouseEvent& event)
{
    if (advancedPanelVisible)
        return;

    const auto dot = advancedDotPosition (getLocalBounds().toFloat());

    if (event.position.getDistanceFrom (dot) < 13.0f && onAdvancedPanelToggle != nullptr)
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
