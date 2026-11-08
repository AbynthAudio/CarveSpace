#include "AdvancedPanel.h"

#include <cmath>

namespace ts
{
namespace
{
    constexpr float kStartAngle = -2.426f;
    constexpr float kEndAngle   =  2.426f;

    const juce::Rectangle<float> titleBox   { 184.9f, 37.9f, 169.5f, 12.5f };
    const juce::Rectangle<float> panLabel   { 64.5f, 86.9f, 25.2f, 9.7f };
    const juce::Rectangle<float> lrLabel    { 148.5f, 86.9f, 23.8f, 9.7f };
    const juce::Rectangle<float> msLabel    { 201.7f, 86.9f, 28.1f, 9.7f };
    const juce::Rectangle<float> attackLabel{ 277.4f, 86.9f, 47.6f, 9.7f };
    const juce::Rectangle<float> releaseLabel{ 347.4f, 86.9f, 56.0f, 9.7f };
    const juce::Rectangle<float> scLabel    { 425.8f, 86.9f, 68.7f, 9.7f };

    const juce::Rectangle<float> panelDesign { 0.0f, 0.0f, 541.0f, 205.0f };

    const juce::Point<float> panCentre    { 77.1f, 140.1f };
    const juce::Point<float> attackCentre { 301.2f, 140.1f };
    const juce::Point<float> releaseCentre{ 376.1f, 140.1f };
    const juce::Point<float> modeCentreL  { 161.8f, 140.1f };
    const juce::Point<float> modeCentreR  { 216.5f, 140.1f };
    const juce::Point<float> scCentre     { 460.9f, 140.1f };
    const juce::Point<float> closeCentre  { 521.8f, 184.2f };
} // namespace

//==============================================================================
AdvancedPanel::RingKnob::RingKnob (juce::RangedAudioParameter& parameter)
    : attachment (parameter, [this] (float newValue)
                  {
                      value = newValue;
                      repaint();
                  })
{
    setInterceptsMouseClicks (true, true);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    attachment.sendInitialUpdate();
}

juce::Point<float> AdvancedPanel::RingKnob::dotPosition (juce::Rectangle<float> bounds, float normalisedValue)
{
    const auto centre = bounds.getCentre();
    const float radius = juce::jmax (1.0f, juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - 5.0f);
    const float angle = kStartAngle + juce::jlimit (0.0f, 1.0f, normalisedValue) * (kEndAngle - kStartAngle);

    return { centre.x + std::sin (angle) * radius,
             centre.y - std::cos (angle) * radius };
}

void AdvancedPanel::RingKnob::setValueFromPosition (juce::Point<float> position)
{
    const auto centre = getLocalBounds().toFloat().getCentre();
    const float dx = position.x - centre.x;
    const float dy = position.y - centre.y;
    const float angle = std::atan2 (dx, -dy);
    const float normalised = juce::jlimit (0.0f, 1.0f, (angle - kStartAngle) / (kEndAngle - kStartAngle));

    attachment.setValueAsCompleteGesture (normalised);
}

void AdvancedPanel::RingKnob::paint (juce::Graphics& g)
{
    const auto& pal = theme::palette();
    const auto bounds = getLocalBounds().toFloat();
    const auto centre = bounds.getCentre();
    const float radius = juce::jmax (1.0f, juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - 5.0f);

    const auto ring = juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre);

    g.setColour (pal.accent.withAlpha (0.14f));
    g.drawEllipse (ring, 4.2f);

    g.setColour (pal.accent);
    g.drawEllipse (ring, 2.0f);

    const auto dot = dotPosition (bounds, value);

    g.setColour (pal.accent.withAlpha (0.22f));
    g.fillEllipse (juce::Rectangle<float> (11.0f, 11.0f).withCentre (dot));

    g.setColour (pal.accent);
    g.fillEllipse (juce::Rectangle<float> (5.6f, 5.6f).withCentre (dot));

    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.fillEllipse (juce::Rectangle<float> (2.2f, 2.2f).withCentre (dot.translated (-0.8f, -0.8f)));
}

void AdvancedPanel::RingKnob::mouseDown (const juce::MouseEvent& event)
{
    dragging = true;
    setValueFromPosition (event.position);
}

void AdvancedPanel::RingKnob::mouseDrag (const juce::MouseEvent& event)
{
    if (dragging)
        setValueFromPosition (event.position);
}

void AdvancedPanel::RingKnob::mouseUp (const juce::MouseEvent&)
{
    dragging = false;
}

void AdvancedPanel::RingKnob::mouseMove (const juce::MouseEvent&) {}
void AdvancedPanel::RingKnob::mouseExit (const juce::MouseEvent&) {}

//==============================================================================
AdvancedPanel::ModeSwitch::ModeSwitch (juce::RangedAudioParameter& parameter)
    : attachment (parameter, [this] (float newValue)
                  {
                      value = newValue;
                      repaint();
                  })
{
    setInterceptsMouseClicks (true, true);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    attachment.sendInitialUpdate();
}

void AdvancedPanel::ModeSwitch::paint (juce::Graphics& g)
{
    const auto& pal = theme::palette();
    const auto bounds = getLocalBounds().toFloat();
    const float cy = bounds.getCentreY();
    const float radius = 12.3f;
    const juce::Point<float> centres[] = { { 13.0f, cy }, { bounds.getWidth() - 13.0f, cy } };

    for (int i = 0; i < 2; ++i)
    {
        const auto circle = juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centres[i]);
        const bool selected = (i == 0) == (value < 0.5f);

        if (selected)
        {
            g.setColour (pal.accent.withAlpha (0.16f));
            g.fillEllipse (circle.expanded (2.5f));
            g.setColour (pal.accent);
            g.fillEllipse (juce::Rectangle<float> (radius * 1.12f, radius * 1.12f).withCentre (centres[i]));
            g.drawEllipse (circle, 2.0f);
        }
        else
        {
            g.setColour (pal.accent.withAlpha (0.85f));
            g.drawEllipse (circle, 1.8f);
        }
    }
}

void AdvancedPanel::ModeSwitch::mouseDown (const juce::MouseEvent& event)
{
    const bool leftHalf = event.position.x < (float) getWidth() * 0.5f;
    attachment.setValueAsCompleteGesture (leftHalf ? 0.0f : 1.0f);
}

void AdvancedPanel::ModeSwitch::mouseMove (const juce::MouseEvent&) {}
void AdvancedPanel::ModeSwitch::mouseExit (const juce::MouseEvent&) {}

//==============================================================================
void AdvancedPanel::RingButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    const auto& pal = theme::palette();
    const auto bounds = getLocalBounds().toFloat();
    const auto centre = bounds.getCentre();
    const float radius = juce::jmax (1.0f, juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - 1.0f);
    const auto ring = juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre);

    if (getToggleState())
    {
        g.setColour (pal.accent.withAlpha (0.16f));
        g.fillEllipse (ring.expanded (2.5f));
        g.setColour (pal.accent);
        g.fillEllipse (juce::Rectangle<float> (radius * 1.05f, radius * 1.05f).withCentre (centre));
    }

    g.setColour (pal.accent.withAlpha (highlighted || down ? 1.0f : 0.88f));
    g.drawEllipse (ring, 1.9f);
}

//==============================================================================
AdvancedPanel::AdvancedPanel (CarveSpaceAudioProcessor& processorToUse,
                              CarveSpaceLookAndFeel& lookAndFeelToUse)
    : processor (processorToUse),
      lookAndFeel (lookAndFeelToUse),
      panKnob (*processor.panParam),
      attackKnob (*processor.attackParam),
      releaseKnob (*processor.releaseParam),
      modeSwitch (*processor.modeParam)
{
    setInterceptsMouseClicks (true, true);
    setOpaque (false);

    addAndMakeVisible (panKnob);
    addAndMakeVisible (attackKnob);
    addAndMakeVisible (releaseKnob);
    addAndMakeVisible (modeSwitch);

    scAuditionButton.setLookAndFeel (&lookAndFeel);
    scAuditionButton.setTooltip ("Listen to the filtered sidechain signal");
    addAndMakeVisible (scAuditionButton);

    scAuditionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.apvts, params::scAudition, scAuditionButton);
}

void AdvancedPanel::resized()
{
    const float sx = (float) getWidth() / panelDesign.getWidth();
    const float sy = (float) getHeight() / panelDesign.getHeight();

    auto placeSquare = [sx, sy] (juce::Point<float> centre, const juce::Component& target) -> juce::Rectangle<int>
    {
        juce::ignoreUnused (target);
        const float side = 66.0f;
        return juce::Rectangle<float> (side, side)
            .withCentre ({ centre.x * sx, centre.y * sy })
            .getSmallestIntegerContainer();
    };

    panKnob.setBounds (placeSquare (panCentre, panKnob));
    attackKnob.setBounds (placeSquare (attackCentre, attackKnob));
    releaseKnob.setBounds (placeSquare (releaseCentre, releaseKnob));

    modeSwitch.setBounds (juce::Rectangle<float> (148.5f, 126.8f, 81.0f, 26.6f)
                              .transformedBy (juce::AffineTransform::scale (sx, sy))
                              .getSmallestIntegerContainer());

    scAuditionButton.setBounds (juce::Rectangle<float> (28.0f, 28.0f)
                                    .withCentre ({ scCentre.x * sx, scCentre.y * sy })
                                    .getSmallestIntegerContainer());
}

void AdvancedPanel::paint (juce::Graphics& g)
{
    const auto& pal = theme::palette();
    const auto bounds = getLocalBounds().toFloat();
    const float corner = layout::displayCorner;
    const float sx = bounds.getWidth() / panelDesign.getWidth();
    const float sy = bounds.getHeight() / panelDesign.getHeight();

    auto scaled = [sx, sy] (juce::Rectangle<float> r) -> juce::Rectangle<float>
    {
        return { r.getX() * sx, r.getY() * sy, r.getWidth() * sx, r.getHeight() * sy };
    };

    juce::Path panel;
    panel.addRoundedRectangle (bounds, corner);

    juce::ColourGradient body (pal.displayHi, bounds.getCentreX(), bounds.getY(),
                               pal.displayLo, bounds.getCentreX(), bounds.getBottom(), false);
    body.addColour (0.45, juce::Colour (0xff1b1b1b));
    g.setGradientFill (body);
    g.fillPath (panel);

    {
        juce::ColourGradient gloss (juce::Colours::white.withAlpha (0.035f), bounds.getCentreX(), bounds.getY(),
                                    juce::Colours::transparentWhite, bounds.getCentreX(), bounds.getY() + 24.0f, false);
        g.setGradientFill (gloss);
        g.fillRect (bounds.withHeight (24.0f));
    }

    g.setColour (juce::Colours::black.withAlpha (0.92f));
    g.strokePath (panel, juce::PathStrokeType (1.1f));

    // Small full-colour brand logo, top-left (keep it subtle).
    {
        const auto& logo = ui::brandLogoImage();

        if (logo.isValid())
        {
            const auto logoArea = scaled ({ 13.0f, 11.0f, 62.0f, 42.0f });
            g.setOpacity (0.92f);
            g.drawImage (logo, logoArea, juce::RectanglePlacement::centred, false);
            g.setOpacity (1.0f);
        }
    }

    // Title and labels.
    ui::drawTrackedText (g, "Advanced Panel", scaled (titleBox), pal.accentText);

    const auto labelColour = pal.accentText.withAlpha (0.92f);
    ui::drawTrackedText (g, "Pan", scaled (panLabel), labelColour);
    ui::drawTrackedText (g, "L/R", scaled (lrLabel), labelColour);
    ui::drawTrackedText (g, "M/S", scaled (msLabel), labelColour);
    ui::drawTrackedText (g, "Attack", scaled (attackLabel), labelColour);
    ui::drawTrackedText (g, "Release", scaled (releaseLabel), labelColour);
    ui::drawTrackedText (g, "Sidechain", scaled (scLabel), labelColour);

    // Close dot.
    const juce::Point<float> dot { closeCentre.x * sx, closeCentre.y * sy };

    g.setColour (pal.accent.withAlpha (0.20f));
    g.fillEllipse (juce::Rectangle<float> (17.0f, 17.0f).withCentre (dot));

    g.setColour (pal.accent);
    g.fillEllipse (juce::Rectangle<float> (9.6f, 9.6f).withCentre (dot));

    g.setColour (juce::Colours::white.withAlpha (0.50f));
    g.fillEllipse (juce::Rectangle<float> (3.0f, 3.0f).withCentre (dot.translated (-1.4f, -1.4f)));

    juce::ignoreUnused (processor);
}

void AdvancedPanel::mouseDown (const juce::MouseEvent& event)
{
    const auto bounds = getLocalBounds().toFloat();
    const float sx = bounds.getWidth() / panelDesign.getWidth();
    const float sy = bounds.getHeight() / panelDesign.getHeight();
    const juce::Point<float> dot { closeCentre.x * sx, closeCentre.y * sy };

    if (event.position.getDistanceFrom (dot) < 13.0f && onClose != nullptr)
        onClose();
}
} // namespace ts
