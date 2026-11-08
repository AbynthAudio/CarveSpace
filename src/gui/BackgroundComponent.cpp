#include "BackgroundComponent.h"

#include <array>

#include "CarveSpaceLookAndFeel.h"

namespace ts
{
BackgroundComponent::BackgroundComponent()
{
    setInterceptsMouseClicks (true, false);
    setOpaque (false);

    // Low-cost swirl rotation: only the small badge area is repainted.
    startTimerHz (60);

    // Very fine grain for the brushed-metal plate.
    constexpr int size = 512;

    grainTexture = juce::Image (juce::Image::ARGB, size, size, true);
    brushedTexture = juce::Image (juce::Image::ARGB, size, size, true);

    juce::Random random (0x5eed1234);

    // Per-column strength for the vertical brushing streaks.
    std::array<float, size> columnStrength {};

    for (auto& value : columnStrength)
        value = 0.25f + 0.75f * random.nextFloat();

    for (int y = 0; y < size; ++y)
    {
        for (int x = 0; x < size; ++x)
        {
            const float speck = random.nextFloat();

            const auto grainAlpha = (juce::uint8) juce::jlimit (0, 255, (int) (speck * speck * 34.0f));
            grainTexture.setPixelAt (x, y, juce::Colour ((juce::uint8) 32, (juce::uint8) 30, (juce::uint8) 28, grainAlpha));

            const float streak = columnStrength[(size_t) (x * 7 % size)] * (0.55f + 0.45f * speck);
            const auto streakAlpha = (juce::uint8) juce::jlimit (0, 255, (int) (streak * 16.0f));
            const bool lightStreak = ((x * 13) % 7) < 3;
            const auto streakColour = lightStreak ? juce::Colour ((juce::uint8) 255, (juce::uint8) 252, (juce::uint8) 246)
                                                  : juce::Colour ((juce::uint8) 40, (juce::uint8) 38, (juce::uint8) 36);
            brushedTexture.setPixelAt (x, y, streakColour.withAlpha (streakAlpha));
        }
    }
}

void BackgroundComponent::drawPlate (juce::Graphics& g)
{
    const auto& pal = theme::palette();
    const auto bounds = getLocalBounds().toFloat();
    const auto centre = bounds.getCentre();
    const float corner = layout::plateCorner + pal.plateCornerExtra;

    g.fillAll (pal.windowBg);

    juce::Path plate;
    plate.addRoundedRectangle (bounds, corner);

    // Base metal gradient: warm and bright at the top-left, cooler and darker
    // towards the bottom-right, exactly like the reference faceplate.
    {
        juce::ColourGradient base (pal.plateHi, 0.0f, 0.0f,
                                   pal.plateLo, bounds.getWidth(), bounds.getHeight(), false);
        base.addColour (0.13, pal.plateHi.interpolatedWith (pal.plateMid, 0.35f));
        base.addColour (0.33, pal.plateMid);
        base.addColour (0.55, pal.plateMid.interpolatedWith (pal.plateLo, 0.45f));
        base.addColour (0.76, pal.plateMid.interpolatedWith (pal.plateLo, 0.80f));
        g.setGradientFill (base);
        g.fillPath (plate);
    }

    // Broad overhead studio light.
    {
        juce::ColourGradient light (pal.plateSheen, centre.x - bounds.getWidth() * 0.12f, -20.0f,
                                    pal.plateSheen.withAlpha (0.0f), centre.x, bounds.getHeight() * 0.55f, true);
        g.setGradientFill (light);
        g.fillPath (plate);
    }

    // Warm tint on the left, cool tint on the right.
    {
        juce::ColourGradient tint (pal.plateWarmTint, 0.0f, bounds.getHeight() * 0.35f,
                                   pal.plateWarmTint.withAlpha (0.0f), bounds.getWidth() * 0.45f, bounds.getHeight() * 0.5f, false);
        g.setGradientFill (tint);
        g.fillPath (plate);
    }

    {
        juce::ColourGradient tint (pal.plateCoolTint.withAlpha (0.0f), bounds.getWidth() * 0.55f, bounds.getHeight() * 0.45f,
                                   pal.plateCoolTint, bounds.getWidth(), bounds.getHeight() * 0.85f, false);
        g.setGradientFill (tint);
        g.fillPath (plate);
    }

    // Brushed texture.
    {
        g.saveState();
        g.reduceClipRegion (plate);

        if (brushedTexture.isValid())
        {
            g.setTiledImageFill (brushedTexture, 0, 0, 0.55f);
            g.fillRect (bounds);
        }

        if (grainTexture.isValid())
        {
            g.setTiledImageFill (grainTexture, 0, 0, 0.65f);
            g.fillRect (bounds);
        }

        g.restoreState();
    }

    // Soft vignette towards the corners / bottom edge.
    {
        juce::ColourGradient vignette (pal.shadow.withAlpha (0.0f), centre.x, centre.y - bounds.getHeight() * 0.10f,
                                       pal.shadow.withAlpha (0.16f), centre.x, bounds.getHeight() * 1.06f, true);
        g.setGradientFill (vignette);
        g.fillPath (plate);
    }

    // Thin bright bevel along the top-left edge and a dark shade bottom-right.
    {
        juce::ColourGradient edge (pal.plateBevelHi, 0.0f, 0.0f,
                                   pal.plateBevelLo, bounds.getWidth(), bounds.getHeight(), false);
        edge.addColour (0.35, pal.plateBevelHi.withAlpha (0.10f));
        edge.addColour (0.65, pal.plateBevelHi.withAlpha (0.12f));
        g.setGradientFill (edge);
        g.strokePath (plate, juce::PathStrokeType (2.0f));
    }

    // Inner hairline (the plate looks milled, not printed).
    {
        const auto inset = juce::AffineTransform::scale (1.0f - 5.0f / bounds.getWidth(),
                                                         1.0f - 5.0f / bounds.getHeight(),
                                                         centre.x, centre.y);
        g.setColour (pal.shadow.withAlpha (0.11f));
        g.strokePath (plate, juce::PathStrokeType (1.0f), inset);
    }

    // The dark display panel sits in a shallow recess with a soft shadow.
    ui::drawSoftShadow (g, layout::display, layout::displayCorner + pal.displayCornerExtra, 9.0f, 0.50f, { 0.0f, 2.0f });
}

void BackgroundComponent::drawLabels (juce::Graphics& g)
{
    const auto& pal = theme::palette();
    const auto ink = pal.ink;

    // Brand line: — CARVESPACE —
    g.setColour (ink);
    g.fillRect (layout::brandDashLeft);
    g.fillRect (layout::brandDashRight);
    ui::drawFittedText (g, "CARVESPACE", layout::brandText, ink);

    // FREEZE caption.
    ui::drawFittedText (g, "FREEZE", layout::freezeLabel, ink);

    // Amount scale.
    g.setColour (ink);
    g.fillEllipse (layout::amountDotLeft);
    g.fillEllipse (layout::amountDotRight);
    ui::drawFittedText (g, "0", layout::amountZero, ink);
    ui::drawFittedText (g, "100", layout::amountHundred, ink);
    ui::drawFittedText (g, "AMOUNT", layout::amountCaption, ink);

    // Low-cut markings.
    g.setColour (ink);
    g.fillEllipse (layout::lowCutDotLeft);
    g.fillEllipse (layout::lowCutDotRight);
    ui::drawFittedText (g, "20 HZ", layout::lowCutMinLabel, ink);
    ui::drawFittedText (g, "20 K", layout::lowCutMaxLabel, ink);
    ui::drawFittedText (g, "LOW-CUT", layout::lowCutCaption, ink);

    // High-cut markings.
    g.setColour (ink);
    g.fillEllipse (layout::highCutDotLeft);
    g.fillEllipse (layout::highCutDotRight);
    ui::drawFittedText (g, "20 HZ", layout::highCutMinLabel, ink);
    ui::drawFittedText (g, "20 K", layout::highCutMaxLabel, ink);
    ui::drawFittedText (g, "HIGH-CUT", layout::highCutCaption, ink);

    // Small angle marks above the cut knobs.
    {
        juce::Path left;
        left.startNewSubPath (layout::bracketLeft.getX(), layout::bracketLeft.getBottom());
        left.lineTo (layout::bracketLeft.getX() + 9.0f, layout::bracketLeft.getY());
        left.lineTo (layout::bracketLeft.getRight(), layout::bracketLeft.getY());

        juce::Path right;
        right.startNewSubPath (layout::bracketRight.getRight(), layout::bracketRight.getBottom());
        right.lineTo (layout::bracketRight.getRight() - 9.0f, layout::bracketRight.getY());
        right.lineTo (layout::bracketRight.getX(), layout::bracketRight.getY());

        const juce::PathStrokeType stroke (2.1f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded);
        g.setColour (ink);
        g.strokePath (left, stroke);
        g.strokePath (right, stroke);
    }

    // Maker wordmark: back to the previous plain sans-serif tracked type (the
    // logo's spiral is kept, but not the logo's wordmark lettering).
    g.setFont (juce::Font ("Arial", 8.5f, juce::Font::plain));
    g.setColour (ink);
    g.drawText ("ABYNTH AUDIO", layout::wordmark, juce::Justification::centred, false);
}

void BackgroundComponent::drawLogo (juce::Graphics& g)
{
    ui::drawLogoBadge (g, layout::logo, logoAngle);
}

void BackgroundComponent::timerCallback()
{
    if (logoPaused || ! isShowing())
        return;

    logoAngle += 0.011f; // clockwise, ~9.5 s per revolution at 60 Hz

    if (logoAngle >= juce::MathConstants<float>::twoPi)
        logoAngle -= juce::MathConstants<float>::twoPi;

    repaint (layout::logo.expanded (3.0f).getSmallestIntegerContainer());
}

void BackgroundComponent::setLogoPaused (bool shouldBePaused)
{
    logoPaused = shouldBePaused;
    repaint (layout::logo.expanded (3.0f).getSmallestIntegerContainer());
}

void BackgroundComponent::paint (juce::Graphics& g)
{
    drawPlate (g);
    drawLabels (g);
    drawLogo (g);
}

void BackgroundComponent::mouseDown (const juce::MouseEvent& event)
{
    if (! layout::logo.contains (event.position))
        return;

    // Right-click opens the info/options menu; left-click stops/resumes the
    // black-hole swirl animation.
    if (event.mods.isRightButtonDown())
    {
        if (onLogoClicked != nullptr)
            onLogoClicked();
    }
    else
    {
        setLogoPaused (! logoPaused);
    }
}

void BackgroundComponent::mouseUp (const juce::MouseEvent&) {}
} // namespace ts
