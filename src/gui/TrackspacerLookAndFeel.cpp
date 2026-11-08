#include "TrackspacerLookAndFeel.h"

namespace ts
{
TrackspacerLookAndFeel::TrackspacerLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, colours::backgroundBottom);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff1b1b1d));
    setColour (juce::PopupMenu::textColourId, juce::Colours::white);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, colours::accent.withAlpha (0.35f));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::Label::textColourId, colours::label);
}

juce::Font TrackspacerLookAndFeel::getLabelFont (juce::Label&)
{
    return juce::Font (juce::Font::getDefaultSansSerifFontName(), 12.0f, juce::Font::plain);
}

void TrackspacerLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                               float sliderPosProportional, float rotaryStartAngle,
                                               float rotaryEndAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (1.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();

    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // Soft drop shadow under the knob.
    g.setColour (juce::Colours::black.withAlpha (0.22f));
    g.fillEllipse (bounds.withSizeKeepingCentre (radius * 2.0f, radius * 2.0f).translated (0.0f, radius * 0.05f));

    // Outer dark ring.
    const auto outer = juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre);
    juce::ColourGradient ring (colours::knobMid, centre.x - radius * 0.6f, centre.y - radius * 0.7f,
                               colours::knobDark, centre.x + radius * 0.7f, centre.y + radius * 0.8f, true);
    ring.addColour (0.5, juce::Colour (0xff2c2c2f));
    g.setGradientFill (ring);
    g.fillEllipse (outer);

    // Machined notches around the rim.
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    for (int i = 0; i < 36; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * (float) i / 36.0f;
        const float inner = radius * 0.90f;
        const float outerR = radius * 0.975f;
        g.drawLine (centre.x + std::sin (a) * inner, centre.y - std::cos (a) * inner,
                    centre.x + std::sin (a) * outerR, centre.y - std::cos (a) * outerR,
                    1.0f);
    }

    // Brushed metal face.
    const float faceRadius = radius * 0.70f;
    const auto face = juce::Rectangle<float> (faceRadius * 2.0f, faceRadius * 2.0f).withCentre (centre);
    juce::ColourGradient faceGradient (juce::Colour (0xfff4f4f4), centre.x - faceRadius * 0.55f, centre.y - faceRadius * 0.55f,
                                       juce::Colour (0xff8d8f92), centre.x + faceRadius * 0.65f, centre.y + faceRadius * 0.7f, true);
    faceGradient.addColour (0.35, juce::Colour (0xffd9dadb));
    faceGradient.addColour (0.62, juce::Colour (0xffb6b8ba));
    g.setGradientFill (faceGradient);
    g.fillEllipse (face);

    // Radial brushing lines.
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    for (int i = 0; i < 60; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * (float) i / 60.0f;
        g.drawLine (centre.x + std::sin (a) * faceRadius * 0.15f, centre.y - std::cos (a) * faceRadius * 0.15f,
                    centre.x + std::sin (a) * faceRadius * 0.97f, centre.y - std::cos (a) * faceRadius * 0.97f,
                    0.5f);
    }

    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.drawEllipse (face, 0.8f);

    // Indicator dot near the rim.
    const float dotDistance = radius * 0.80f;
    const juce::Point<float> dot (centre.x + std::sin (angle) * dotDistance,
                                  centre.y - std::cos (angle) * dotDistance);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (juce::Rectangle<float> (radius * 0.11f, radius * 0.11f).withCentre (dot).translated (0.5f, 0.5f));
    g.setColour (juce::Colour (0xfff5f5f5));
    g.fillEllipse (juce::Rectangle<float> (radius * 0.095f, radius * 0.095f).withCentre (dot));

    juce::ignoreUnused (slider);
}

void TrackspacerLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                               bool shouldDrawButtonAsHighlighted,
                                               bool shouldDrawButtonAsDown)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced (2.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const bool on = button.getToggleState();

    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillEllipse (bounds.withSizeKeepingCentre (radius * 2.0f, radius * 2.0f).translated (0.0f, 1.5f));

    juce::ColourGradient metal (juce::Colour (0xffe6e6e6), centre.x - radius * 0.5f, centre.y - radius * 0.5f,
                                juce::Colour (0xff8b8b8b), centre.x + radius * 0.5f, centre.y + radius * 0.5f, true);
    g.setGradientFill (metal);
    g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre));

    g.setColour (juce::Colour (0xff2a2a2a));
    g.drawEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre), 1.2f);

    const float innerRadius = radius * 0.55f;
    g.setColour (on ? colours::accent : juce::Colour (0xff4a4a4a));
    g.fillEllipse (juce::Rectangle<float> (innerRadius * 2.0f, innerRadius * 2.0f).withCentre (centre));
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillEllipse (juce::Rectangle<float> (innerRadius * 1.1f, innerRadius * 1.1f).withCentre (centre));

    if (shouldDrawButtonAsHighlighted && ! on)
    {
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre));
    }

    juce::ignoreUnused (shouldDrawButtonAsDown);
}
} // namespace ts
