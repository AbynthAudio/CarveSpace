#include "PluginEditor.h"

namespace ts
{
TrackspacerAudioProcessorEditor::TrackspacerAudioProcessorEditor (TrackspacerAudioProcessor& processorToUse)
    : juce::AudioProcessorEditor (&processorToUse),
      processor (processorToUse),
      spectrumDisplay (processorToUse),
      advancedPanel (processorToUse, lookAndFeel)
{
    setLookAndFeel (&lookAndFeel);
    setOpaque (true);
    setWantsKeyboardFocus (true);

    addAndMakeVisible (content);
    content.setOpaque (false);

    content.addAndMakeVisible (background);
    content.addAndMakeVisible (spectrumDisplay);
    content.addChildComponent (advancedPanel);
    advancedPanel.setVisible (false);

    content.addAndMakeVisible (amountKnob);
    content.addAndMakeVisible (lowCutKnob);
    content.addAndMakeVisible (highCutKnob);

    freezeButton.setLookAndFeel (&lookAndFeel);
    freezeButton.setButtonText ("");
    freezeButton.setTooltip ("Freeze the current reduction curve");
    content.addAndMakeVisible (freezeButton);

    amountKnob.setTooltip ("Amount of reduction applied to the main signal");
    lowCutKnob.setTooltip ("Low-cut filter for the processed range");
    highCutKnob.setTooltip ("High-cut filter for the processed range");

    amountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, params::amount, amountKnob);
    lowCutAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, params::lowCut, lowCutKnob);
    highCutAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, params::highCut, highCutKnob);
    freezeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.apvts, params::freeze, freezeButton);

    amountKnob.setDoubleClickReturnValue (true, params::amountDefault);
    lowCutKnob.setDoubleClickReturnValue (true, params::lowCutDefault);
    highCutKnob.setDoubleClickReturnValue (true, params::highCutDefault);

    spectrumDisplay.onAdvancedPanelToggle = [this] { setAdvancedPanelOpen (true); };
    advancedPanel.onClose = [this] { setAdvancedPanelOpen (false); };
    background.onLogoClicked = [this] { showInfoMenu(); };

    setSize (designWidth, designHeight);
    setResizable (true, true);
    setResizeLimits (325, 325, 1300, 1300);
    if (auto* constrainer = getConstrainer())
        constrainer->setFixedAspectRatio (1.0);

    resized();
}

TrackspacerAudioProcessorEditor::~TrackspacerAudioProcessorEditor()
{
    amountAttachment.reset();
    lowCutAttachment.reset();
    highCutAttachment.reset();
    freezeAttachment.reset();

    freezeButton.setLookAndFeel (nullptr);
    amountKnob.setLookAndFeel (nullptr);
    lowCutKnob.setLookAndFeel (nullptr);
    highCutKnob.setLookAndFeel (nullptr);
    advancedPanel.setVisible (false);
    setLookAndFeel (nullptr);
}

juce::Rectangle<float> TrackspacerAudioProcessorEditor::designBoundsFor (int x, int y, int w, int h) const
{
    return juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
}

void TrackspacerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1b1b1d));
}

void TrackspacerAudioProcessorEditor::resized()
{
    const float scale = juce::jmin ((float) getWidth() / (float) designWidth,
                                    (float) getHeight() / (float) designHeight);
    const float offsetX = ((float) getWidth() - (float) designWidth * scale) * 0.5f;
    const float offsetY = ((float) getHeight() - (float) designHeight * scale) * 0.5f;

    content.setBounds (0, 0, designWidth, designHeight);
    content.setTransform (juce::AffineTransform::scale (scale).translated (offsetX, offsetY));

    background.setBounds (0, 0, designWidth, designHeight);
    spectrumDisplay.setBounds (50, 57, 548, 196);
    advancedPanel.setBounds (50, 57, 548, 196);

    amountKnob.setBounds (223, 258, 204, 204);
    lowCutKnob.setBounds (51, 437, 137, 137);
    highCutKnob.setBounds (457, 433, 145, 145);
    freezeButton.setBounds (92, 317, 36, 36);
}

void TrackspacerAudioProcessorEditor::setAdvancedPanelOpen (bool shouldOpen)
{
    processor.setAdvancedPanelOpen (shouldOpen);
    advancedPanel.setVisible (shouldOpen);
    spectrumDisplay.setAdvancedPanelVisible (shouldOpen);

    if (shouldOpen)
        advancedPanel.toFront (true);
    else
        spectrumDisplay.repaint();
}

void TrackspacerAudioProcessorEditor::showInfoMenu()
{
    juce::PopupMenu menu;
    menu.addItem (1, "About Trackspacer Clone");
    menu.addItem (2, "Visit website");
    menu.addItem (3, "Default size");
    menu.addSeparator();
    menu.addItem (4, "Read the manual");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&background),
                        [this] (int result)
                        {
                            switch (result)
                            {
                                case 1:
                                    juce::AlertWindow::showMessageBoxAsync (
                                        juce::MessageBoxIconType::InfoIcon,
                                        "Trackspacer Clone 1.0.0",
                                        "A native Linux functional clone of Wavesfactory Trackspacer 2.5.\n\n"
                                        "32-band spectral sidechain carver: analyses the sidechain and applies "
                                        "the inverse EQ curve to the main signal.\n\n"
                                        "Built with JUCE 7.0.5. Frontend and DSP written from scratch.\n"
                                        "Trackspacer is a trademark of Wavesfactory. This is an independent "
                                        "compatibility reimplementation.");
                                    break;

                                case 2:
                                    juce::URL ("https://www.wavesfactory.com/audio-plugins/trackspacer/")
                                        .launchInDefaultBrowser();
                                    break;

                                case 3:
                                    if (auto* constrainer = getConstrainer())
                                        constrainer->setFixedAspectRatio (1.0);
                                    setSize (designWidth, designHeight);
                                    break;

                                case 4:
                                    juce::URL ("https://www.wavesfactory.com/audio-plugins/manuals/Trackspacer-User-Manual.pdf")
                                        .launchInDefaultBrowser();
                                    break;

                                default:
                                    break;
                            }
                        });
}
} // namespace ts
