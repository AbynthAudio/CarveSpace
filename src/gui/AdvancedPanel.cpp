#include "AdvancedPanel.h"

namespace ts
{
AdvancedPanel::ModeSwitch::ModeSwitch (juce::RangedAudioParameter& parameter)
    : attachment (parameter, [this] (float value)
                  {
                      currentValue = value;
                      repaint();
                  })
{
    setInterceptsMouseClicks (true, true);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    attachment.sendInitialUpdate();
}

void AdvancedPanel::ModeSwitch::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const juce::Point<float> centres[] = { { bounds.getX() + bounds.getWidth() * 0.25f, bounds.getCentreY() },
                                           { bounds.getX() + bounds.getWidth() * 0.75f, bounds.getCentreY() } };

    for (int i = 0; i < 2; ++i)
    {
        const bool selected = (i == 0 && currentValue < 0.5f) || (i == 1 && currentValue >= 0.5f);
        const float radius = 6.0f;

        g.setColour (selected ? colours::accent : juce::Colour (0xff5a5c60));
        g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centres[i]));

        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centres[i]), 1.0f);
    }
}

void AdvancedPanel::ModeSwitch::mouseDown (const juce::MouseEvent& event)
{
    const auto bounds = getLocalBounds().toFloat();
    const bool leftHalf = event.position.x < bounds.getCentreX();
    attachment.setValueAsCompleteGesture (leftHalf ? 0.0f : 1.0f);
}

AdvancedPanel::AdvancedPanel (TrackspacerAudioProcessor& processorToUse,
                              TrackspacerLookAndFeel& lookAndFeelToUse)
    : processor (processorToUse),
      lookAndFeel (lookAndFeelToUse),
      modeSwitch (*processor.modeParam)
{
    setInterceptsMouseClicks (true, true);
    setOpaque (false);

    addAndMakeVisible (panKnob);
    addAndMakeVisible (attackKnob);
    addAndMakeVisible (releaseKnob);
    addAndMakeVisible (modeSwitch);

    scAuditionButton.setLookAndFeel (&lookAndFeel);
    scAuditionButton.setButtonText ("");
    scAuditionButton.setTooltip ("SC Audition: listen to the filtered sidechain");
    addAndMakeVisible (scAuditionButton);

    panKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    attackKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    releaseKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);

    panKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    attackKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    releaseKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);

    panAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, params::pan, panKnob);
    attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, params::attack, attackKnob);
    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, params::release, releaseKnob);
    scAuditionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.apvts, params::scAudition, scAuditionButton);
}

void AdvancedPanel::resized()
{
    const auto width = getWidth();
    const int controlSize = 54;
    const int centreY = juce::roundToInt (getHeight() * 0.60f);

    panKnob.setBounds (juce::roundToInt (width * 0.135f) - controlSize / 2, centreY - controlSize / 2, controlSize, controlSize);
    attackKnob.setBounds (juce::roundToInt (width * 0.52f) - controlSize / 2, centreY - controlSize / 2, controlSize, controlSize);
    releaseKnob.setBounds (juce::roundToInt (width * 0.695f) - controlSize / 2, centreY - controlSize / 2, controlSize, controlSize);

    modeSwitch.setBounds (juce::roundToInt (width * 0.345f), centreY - 10, 64, 20);

    scAuditionButton.setBounds (juce::roundToInt (width * 0.855f) - 15, centreY - 15, 30, 30);
}

void AdvancedPanel::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float corner = 9.0f;

    g.setColour (juce::Colour (0xff101113));
    g.fillRoundedRectangle (bounds, corner);
    g.setColour (juce::Colour (0xff2f3236));
    g.drawRoundedRectangle (bounds.reduced (0.5f), corner, 1.0f);

    const auto titleFont = juce::Font (juce::Font::getDefaultSansSerifFontName(), 13.0f, juce::Font::plain);
    g.setFont (titleFont);
    g.setColour (juce::Colour (0xff9fc4ff));
    g.drawText ("Advanced Panel", juce::Rectangle<float> (0.0f, 10.0f, bounds.getWidth(), 18.0f), juce::Justification::centred);

    const auto labelFont = juce::Font (juce::Font::getDefaultSansSerifFontName(), 9.5f, juce::Font::plain);
    g.setFont (labelFont);
    g.setColour (juce::Colour (0xffa7b6c8));

    const float width = bounds.getWidth();
    const float labelY = 62.0f;

    g.drawText ("Pan", juce::Rectangle<float> (width * 0.135f - 30.0f, labelY, 60.0f, 12.0f), juce::Justification::centred);
    g.drawText ("L/R", juce::Rectangle<float> (width * 0.345f + 2.0f, labelY, 24.0f, 12.0f), juce::Justification::centred);
    g.drawText ("M/S", juce::Rectangle<float> (width * 0.345f + 38.0f, labelY, 24.0f, 12.0f), juce::Justification::centred);
    g.drawText ("Attack", juce::Rectangle<float> (width * 0.52f - 30.0f, labelY, 60.0f, 12.0f), juce::Justification::centred);
    g.drawText ("Release", juce::Rectangle<float> (width * 0.695f - 30.0f, labelY, 60.0f, 12.0f), juce::Justification::centred);
    g.drawText ("Sidechain", juce::Rectangle<float> (width * 0.855f - 40.0f, labelY, 80.0f, 12.0f), juce::Justification::centred);

    // Close / back dot.
    const juce::Point<float> dot (bounds.getRight() - 20.0f, bounds.getBottom() - 15.0f);
    g.setColour (colours::accent.withAlpha (0.35f));
    g.fillEllipse (juce::Rectangle<float> (11.0f, 11.0f).withCentre (dot));
    g.setColour (colours::accent);
    g.fillEllipse (juce::Rectangle<float> (6.0f, 6.0f).withCentre (dot));

    juce::ignoreUnused (processor);
}

void AdvancedPanel::mouseDown (const juce::MouseEvent& event)
{
    const auto bounds = getLocalBounds().toFloat();
    const juce::Point<float> dot (bounds.getRight() - 20.0f, bounds.getBottom() - 15.0f);

    if (event.position.getDistanceFrom (dot) < 14.0f)
        if (onClose != nullptr)
            onClose();
}
} // namespace ts
