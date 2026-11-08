#include "BackgroundComponent.h"

#include "TrackspacerLookAndFeel.h"

namespace ts
{
BackgroundComponent::BackgroundComponent()
{
    setInterceptsMouseClicks (true, false);
    setOpaque (false);

    // Subtle grain so the light plate does not look flat.
    noiseTexture = juce::Image (juce::Image::ARGB, 256, 256, true);
    juce::Random random (0x5eed1234);

    for (int y = 0; y < noiseTexture.getHeight(); ++y)
    {
        for (int x = 0; x < noiseTexture.getWidth(); ++x)
        {
            const auto value = random.nextFloat();
            const auto alpha = value > 0.75f ? (juce::uint8) (value * 22.0f) : (juce::uint8) 0;
            noiseTexture.setPixelAt (x, y, juce::Colour ((juce::uint8) 30, (juce::uint8) 30, (juce::uint8) 30, alpha));
        }
    }
}

void BackgroundComponent::drawPlate (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced (1.5f);
    const float corner = 10.0f;

    g.setColour (colours::border);
    g.fillRoundedRectangle (bounds, corner);

    auto plate = bounds.reduced (2.0f);

    juce::ColourGradient plateGradient (colours::backgroundTop, plate.getCentreX() - plate.getWidth() * 0.45f, plate.getY(),
                                        colours::backgroundBottom, plate.getCentreX() + plate.getWidth() * 0.45f, plate.getBottom(), false);
    plateGradient.addColour (0.35, juce::Colour (0xffdedcd8));
    plateGradient.addColour (0.7, juce::Colour (0xffd2d0cc));
    g.setGradientFill (plateGradient);
    g.fillRoundedRectangle (plate, corner - 2.0f);

    // Very light top highlight to suggest a brushed metal surface.
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.35f),
                                             plate.getCentreX(), plate.getY(),
                                             juce::Colours::transparentWhite,
                                             plate.getCentreX(), plate.getY() + plate.getHeight() * 0.18f,
                                             false));
    g.fillRoundedRectangle (plate.withHeight (plate.getHeight() * 0.2f), corner - 2.0f);

    if (noiseTexture.isValid())
    {
        g.setTiledImageFill (noiseTexture, 0, 0, 0.45f);
        g.fillRoundedRectangle (plate, corner - 2.0f);
    }

    g.setColour (juce::Colours::white.withAlpha (0.18f));
    g.drawRoundedRectangle (plate.reduced (0.6f), corner - 2.0f, 1.0f);
}

void BackgroundComponent::drawLabels (juce::Graphics& g)
{
    const float scale = (float) getWidth() / 650.0f;
    juce::ignoreUnused (scale);

    const auto labelFont = juce::Font (juce::Font::getDefaultSansSerifFontName(), 12.0f, juce::Font::plain);
    const auto smallFont = juce::Font (juce::Font::getDefaultSansSerifFontName(), 10.0f, juce::Font::bold);
    const auto titleFont = juce::Font (juce::Font::getDefaultSansSerifFontName(), 33.0f, juce::Font::plain);

    g.setFont (labelFont);
    g.setColour (colours::label);

    // FREEZE label + TRACKSPACER branding.
    g.drawText ("FREEZE", juce::Rectangle<float> (70.0f, 276.0f, 80.0f, 16.0f), juce::Justification::centred);
    g.drawText ("\u2014 TRACKSPACER \u2014", juce::Rectangle<float> (430.0f, 276.0f, 165.0f, 16.0f), juce::Justification::centred);

    // Amount scale.
    g.setFont (smallFont);
    g.drawText ("0", juce::Rectangle<float> (232.0f, 486.0f, 30.0f, 14.0f), juce::Justification::centred);
    g.drawText ("100", juce::Rectangle<float> (388.0f, 486.0f, 30.0f, 14.0f), juce::Justification::centred);

    g.setColour (juce::Colour (0xff3a3a3c));
    g.fillEllipse (239.0f, 471.0f, 5.0f, 5.0f);
    g.fillEllipse (395.0f, 471.0f, 5.0f, 5.0f);

    // Knob captions and scale labels.
    g.setFont (labelFont);
    g.setColour (colours::label);
    g.drawText ("AMOUNT", juce::Rectangle<float> (265.0f, 470.0f, 120.0f, 16.0f), juce::Justification::centred);

    g.setFont (smallFont);
    g.drawText ("20 HZ", juce::Rectangle<float> (58.0f, 556.0f, 45.0f, 12.0f), juce::Justification::centred);
    g.drawText ("20 K", juce::Rectangle<float> (142.0f, 556.0f, 45.0f, 12.0f), juce::Justification::centred);
    g.drawText ("LOW-CUT", juce::Rectangle<float> (70.0f, 570.0f, 100.0f, 14.0f), juce::Justification::centred);

    g.drawText ("20 HZ", juce::Rectangle<float> (468.0f, 556.0f, 45.0f, 12.0f), juce::Justification::centred);
    g.drawText ("20 K", juce::Rectangle<float> (552.0f, 556.0f, 45.0f, 12.0f), juce::Justification::centred);
    g.drawText ("HIGH-CUT", juce::Rectangle<float> (480.0f, 570.0f, 100.0f, 14.0f), juce::Justification::centred);

    // Small corner brackets over the low / high cut knobs.
    g.setColour (juce::Colour (0xff3d3d3f));
    const float bracketY = 430.0f;
    g.drawLine (88.0f, bracketY, 108.0f, bracketY, 1.6f);
    g.drawLine (88.0f, bracketY, 88.0f, bracketY + 6.0f, 1.6f);
    g.drawLine (498.0f, bracketY, 518.0f, bracketY, 1.6f);
    g.drawLine (518.0f, bracketY, 518.0f, bracketY + 6.0f, 1.6f);

    // Wavesfactory wordmark.
    g.setFont (juce::Font (juce::Font::getDefaultSansSerifFontName(), 11.0f, juce::Font::plain));
    g.setColour (juce::Colour (0xff4a4a4c));
    g.drawText ("WAVESFACTORY", juce::Rectangle<float> (215.0f, 598.0f, 220.0f, 16.0f), juce::Justification::centred);

    juce::ignoreUnused (titleFont);
}

void BackgroundComponent::drawLogo (juce::Graphics& g)
{
    // Circular Wavesfactory "W" badge (recreated, not the original artwork).
    const juce::Rectangle<float> badge (303.0f, 548.0f, 44.0f, 44.0f);
    g.setColour (juce::Colours::black.withAlpha (0.22f));
    g.fillEllipse (badge.translated (0.0f, 1.5f));
    g.setColour (juce::Colour (0xff2c2c2e));
    g.fillEllipse (badge);
    g.setColour (juce::Colour (0xff444446));
    g.drawEllipse (badge.reduced (1.0f), 1.0f);

    g.setFont (juce::Font (juce::Font::getDefaultSansSerifFontName(), 25.0f, juce::Font::italic | juce::Font::bold));
    g.setColour (juce::Colour (0xffe8e8e8));
    g.drawText ("W", badge.reduced (2.0f), juce::Justification::centred);
}

void BackgroundComponent::paint (juce::Graphics& g)
{
    drawPlate (g);
    drawLabels (g);
    drawLogo (g);
}

void BackgroundComponent::mouseDown (const juce::MouseEvent& event)
{
    // The logo opens the info / options menu.
    if (juce::Rectangle<float> (303.0f, 548.0f, 44.0f, 44.0f).contains (event.position))
        if (onLogoClicked != nullptr)
            onLogoClicked();
}
} // namespace ts
