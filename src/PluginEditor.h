#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"
#include "gui/AdvancedPanel.h"
#include "gui/BackgroundComponent.h"
#include "gui/CarveSpaceLookAndFeel.h"
#include "gui/ParameterKnob.h"
#include "gui/SpectrumDisplay.h"

namespace ts
{
class CarveSpaceAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                             private juce::Timer
{
public:
    explicit CarveSpaceAudioProcessorEditor (CarveSpaceAudioProcessor& processorToUse);
    ~CarveSpaceAudioProcessorEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    static constexpr int designWidth = layout::designWidth;
    static constexpr int designHeight = layout::designHeight;

private:
    void setAdvancedPanelOpen (bool shouldOpen);
    void showInfoMenu();
    void showAboutDialog();

    void applyTheme (ThemeId newTheme);
    void persistThemeChoice (ThemeId newTheme);
    void repaintEverything();

    void cycleTheme();
    void showThemeMenu (juce::Component* target);
    juce::String currentThemeName() const;

    void applyEnvironmentParameterOverrides();

    void timerCallback() override;

    CarveSpaceAudioProcessor& processor;
    CarveSpaceLookAndFeel lookAndFeel;
    juce::TooltipWindow tooltipWindow { this, 700 };

    juce::Component content;
    BackgroundComponent background;
    SpectrumDisplay spectrumDisplay;
    AdvancedPanel advancedPanel;

    ParameterKnob amountKnob { "Amount", &lookAndFeel };
    ParameterKnob lowCutKnob { "Low Cut", &lookAndFeel };
    ParameterKnob highCutKnob { "High Cut", &lookAndFeel };
    juce::ToggleButton freezeButton { "Freeze" };
    SkinButton skinButton;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> amountAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lowCutAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> highCutAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> freezeAttachment;

    ThemeId currentTheme = ThemeId::dark;
    bool themeLockedByEnvironment = false;
    juce::Point<int> pendingEditorSize { 0, 0 };
    bool editorSizeDirty = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CarveSpaceAudioProcessorEditor)
};
} // namespace ts
