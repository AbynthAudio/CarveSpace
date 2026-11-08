#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"
#include "gui/AdvancedPanel.h"
#include "gui/BackgroundComponent.h"
#include "gui/ParameterKnob.h"
#include "gui/SpectrumDisplay.h"
#include "gui/TrackspacerLookAndFeel.h"

namespace ts
{
class TrackspacerAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit TrackspacerAudioProcessorEditor (TrackspacerAudioProcessor& processorToUse);
    ~TrackspacerAudioProcessorEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    static constexpr int designWidth = 650;
    static constexpr int designHeight = 650;

private:
    void setAdvancedPanelOpen (bool shouldOpen);
    void showInfoMenu();

    TrackspacerAudioProcessor& processor;
    TrackspacerLookAndFeel lookAndFeel;
    juce::TooltipWindow tooltipWindow { this, 700 };

    juce::Component content;
    BackgroundComponent background;
    SpectrumDisplay spectrumDisplay;
    AdvancedPanel advancedPanel;

    ParameterKnob amountKnob { "Amount", &lookAndFeel };
    ParameterKnob lowCutKnob { "Low Cut", &lookAndFeel };
    ParameterKnob highCutKnob { "High Cut", &lookAndFeel };
    juce::ToggleButton freezeButton { "Freeze" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> amountAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lowCutAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> highCutAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> freezeAttachment;

    juce::Rectangle<float> designBoundsFor (int x, int y, int w, int h) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackspacerAudioProcessorEditor)
};
} // namespace ts
