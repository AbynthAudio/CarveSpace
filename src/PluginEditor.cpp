#include "PluginEditor.h"

namespace ts
{
CarveSpaceAudioProcessorEditor::CarveSpaceAudioProcessorEditor (CarveSpaceAudioProcessor& processorToUse)
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

    amountKnob.setDiscScale (layout::amountDiscScale);
    lowCutKnob.setDiscScale (layout::smallDiscScale);
    highCutKnob.setDiscScale (layout::smallDiscScale);

    content.addAndMakeVisible (amountKnob);
    content.addAndMakeVisible (lowCutKnob);
    content.addAndMakeVisible (highCutKnob);

    freezeButton.setLookAndFeel (&lookAndFeel);
    freezeButton.setButtonText ("");
    freezeButton.setTooltip ("Freeze the current reduction curve");
    freezeButton.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    content.addAndMakeVisible (freezeButton);

    amountKnob.setTooltip ("Amount of reduction applied to the main signal");
    lowCutKnob.setTooltip ("Low-cut: lower limit of the processed range");
    highCutKnob.setTooltip ("High-cut: upper limit of the processed range");

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

    // Always-visible skin switcher (left click cycles, right click opens the list).
    content.addAndMakeVisible (skinButton);
    skinButton.onClick = [this] { cycleTheme(); };
    skinButton.onSecondaryClick = [this] { showThemeMenu (&skinButton); };

    applyEnvironmentParameterOverrides();

    spectrumDisplay.onAdvancedPanelToggle = [this] { setAdvancedPanelOpen (true); };
    advancedPanel.onClose = [this] { setAdvancedPanelOpen (false); };
    background.onLogoClicked = [this] { showInfoMenu(); };

    setResizable (true, true);
    setResizeLimits (325, 325, 1300, 1300);

    if (auto* boundsConstrainer = getConstrainer())
        boundsConstrainer->setFixedAspectRatio (1.0);

    // ---- restored editor size (square, 325 .. 1300) ------------------------
    {
        const int savedWidth = processor.getSavedEditorWidth();
        const int savedHeight = processor.getSavedEditorHeight();
        const int side = juce::jlimit (325, 1300, juce::jmax (savedWidth, savedHeight));

        setSize (side, side);
        pendingEditorSize = { side, side };
    }

    // ---- theme (session state first, else user config, else dark) ----------
    ThemeId initialTheme = theme::fromString (processor.getUiTheme());

    bool themeForcedByEnvironment = false;
    const auto envTheme = juce::SystemStats::getEnvironmentVariable ("CARVESPACE_THEME", {});

    if (envTheme.isNotEmpty())
    {
        initialTheme = theme::fromString (envTheme);
        themeForcedByEnvironment = true;
    }

    themeLockedByEnvironment = themeForcedByEnvironment;

    applyTheme (initialTheme);

    if (! themeForcedByEnvironment)
        processor.setUiTheme (theme::toString (initialTheme));

    // Test hook: force a specific editor size (e.g. CARVESPACE_EDITOR_SIZE=975).
    {
        const auto sizeText = juce::SystemStats::getEnvironmentVariable ("CARVESPACE_EDITOR_SIZE", {});

        if (sizeText.isNotEmpty())
        {
            const int side = juce::jlimit (325, 1300, sizeText.getIntValue());

            if (side > 0)
            {
                setSize (side, side);
                pendingEditorSize = { side, side };
            }
        }
    }

    // Test hook: persist the current UI state immediately (persistence testing).
    if (juce::SystemStats::getEnvironmentVariable ("CARVESPACE_PERSIST_UI", {}) == "1")
    {
        themeLockedByEnvironment = false;
        persistThemeChoice (initialTheme);
    }

    // Screenshot / testing hook: open the advanced panel on construction.
    const auto advancedFlag = juce::SystemStats::getEnvironmentVariable ("CARVESPACE_SHOW_ADVANCED", {});
    const auto legacyFlag = juce::SystemStats::getEnvironmentVariable ("TRACKSPACER_SHOW_ADVANCED", {});

    if (advancedFlag == "1" || legacyFlag == "1")
        setAdvancedPanelOpen (true);

    // Screenshot hook for the About dialog.
    if (juce::SystemStats::getEnvironmentVariable ("CARVESPACE_SHOW_ABOUT", {}) == "1")
        juce::MessageManager::callAsync ([this] { showAboutDialog(); });

    startTimer (600);
    resized();
}

CarveSpaceAudioProcessorEditor::~CarveSpaceAudioProcessorEditor()
{
    stopTimer();

    if (editorSizeDirty)
        processor.saveEditorSize (pendingEditorSize.x, pendingEditorSize.y);

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

void CarveSpaceAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (theme::palette().windowBg);
}

void CarveSpaceAudioProcessorEditor::resized()
{
    const float scale = juce::jmin ((float) getWidth() / (float) designWidth,
                                    (float) getHeight() / (float) designHeight);

    const float offsetX = ((float) getWidth() - (float) designWidth * scale) * 0.5f;
    const float offsetY = ((float) getHeight() - (float) designHeight * scale) * 0.5f;

    content.setBounds (0, 0, designWidth, designHeight);
    content.setTransform (juce::AffineTransform::scale (scale).translated (offsetX, offsetY));

    background.setBounds (0, 0, designWidth, designHeight);
    spectrumDisplay.setBounds (layout::display.toNearestInt());
    advancedPanel.setBounds (layout::display.toNearestInt());

    amountKnob.setBounds (layout::amountKnob.toNearestInt());
    lowCutKnob.setBounds (layout::lowCutKnob.toNearestInt());
    highCutKnob.setBounds (layout::highCutKnob.toNearestInt());
    freezeButton.setBounds (layout::freezeButton.toNearestInt());
    skinButton.setBounds (layout::skinButton.toNearestInt());

    // The processor persists the window size (debounced in timerCallback).
    pendingEditorSize = { getWidth(), getHeight() };
    editorSizeDirty = true;
}

//==============================================================================

void CarveSpaceAudioProcessorEditor::applyTheme (ThemeId newTheme)
{
    theme::setTheme (newTheme);
    currentTheme = newTheme;
    lookAndFeel.refreshTheme();
    skinButton.setTooltip ("Theme: " + currentThemeName() + " - click to cycle Dark / Light / Pink");
    repaintEverything();
}

juce::String CarveSpaceAudioProcessorEditor::currentThemeName() const
{
    const auto name = theme::toString (currentTheme);
    return name.substring (0, 1).toUpperCase() + name.substring (1);
}

void CarveSpaceAudioProcessorEditor::cycleTheme()
{
    ThemeId next = ThemeId::dark;

    switch (currentTheme)
    {
        case ThemeId::dark:  next = ThemeId::light; break;
        case ThemeId::light: next = ThemeId::pink;  break;
        case ThemeId::pink:  next = ThemeId::dark;  break;
    }

    applyTheme (next);
    persistThemeChoice (next);
}

void CarveSpaceAudioProcessorEditor::applyEnvironmentParameterOverrides()
{
    // Test / screenshot hooks: drive parameters to specific values.
    auto setParam = [this] (const char* envName, juce::RangedAudioParameter* parameter)
    {
        if (parameter == nullptr)
            return;

        const auto text = juce::SystemStats::getEnvironmentVariable (envName, {});

        if (text.isEmpty())
            return;

        const float realValue = text.getFloatValue();
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (realValue));
    };

    setParam ("CARVESPACE_AMOUNT", processor.amountParam);
    setParam ("CARVESPACE_LOWCUT", processor.lowCutParam);
    setParam ("CARVESPACE_HIGHCUT", processor.highCutParam);
    setParam ("CARVESPACE_PAN", processor.panParam);
    setParam ("CARVESPACE_ATTACK", processor.attackParam);
    setParam ("CARVESPACE_RELEASE", processor.releaseParam);

    const auto freezeFlag = juce::SystemStats::getEnvironmentVariable ("CARVESPACE_FREEZE", {});

    if (freezeFlag == "1" && processor.freezeParam != nullptr)
        processor.freezeParam->setValueNotifyingHost (1.0f);
}

void CarveSpaceAudioProcessorEditor::persistThemeChoice (ThemeId newTheme)
{
    themeLockedByEnvironment = false;
    processor.setUiTheme (theme::toString (newTheme));
    processor.saveEditorSize (getWidth(), getHeight());
    editorSizeDirty = false;
}

void CarveSpaceAudioProcessorEditor::repaintEverything()
{
    background.repaint();
    spectrumDisplay.repaint();
    advancedPanel.repaint();
    amountKnob.repaint();
    lowCutKnob.repaint();
    highCutKnob.repaint();
    freezeButton.repaint();
    content.repaint();
    repaint();
}

void CarveSpaceAudioProcessorEditor::timerCallback()
{
    // Pick up a theme restored later by the host (session state can be loaded
    // after the editor has already been created).
    if (! themeLockedByEnvironment)
    {
        const auto restored = theme::fromString (processor.getUiTheme());

        if (restored != currentTheme)
            applyTheme (restored);
    }

    // Debounced editor-size persistence (avoids writing the config on every
    // pixel of a window drag).
    if (editorSizeDirty)
    {
        processor.saveEditorSize (pendingEditorSize.x, pendingEditorSize.y);
        editorSizeDirty = false;
    }
}

//==============================================================================
void CarveSpaceAudioProcessorEditor::setAdvancedPanelOpen (bool shouldOpen)
{
    processor.setAdvancedPanelOpen (shouldOpen);
    advancedPanel.setVisible (shouldOpen);
    spectrumDisplay.setAdvancedPanelVisible (shouldOpen);

    if (shouldOpen)
        advancedPanel.toFront (true);
    else
        spectrumDisplay.repaint();
}

void CarveSpaceAudioProcessorEditor::showThemeMenu (juce::Component* target)
{
    juce::PopupMenu themeMenu;
    themeMenu.addItem (100, "Dark", true, currentTheme == ThemeId::dark);
    themeMenu.addItem (101, "Light", true, currentTheme == ThemeId::light);
    themeMenu.addItem (102, "Pink", true, currentTheme == ThemeId::pink);

    themeMenu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (target),
                             [this] (int result)
                             {
                                 switch (result)
                                 {
                                     case 100: applyTheme (ThemeId::dark);  persistThemeChoice (ThemeId::dark);  break;
                                     case 101: applyTheme (ThemeId::light); persistThemeChoice (ThemeId::light); break;
                                     case 102: applyTheme (ThemeId::pink);  persistThemeChoice (ThemeId::pink);  break;
                                     default: break;
                                 }
                             });
}

void CarveSpaceAudioProcessorEditor::showAboutDialog()
{
    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned (new AboutContent());
    options.dialogTitle = "About CarveSpace";
    options.dialogBackgroundColour = juce::Colour (0xff16121c);
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;
    options.componentToCentreAround = this;
    options.launchAsync();
}

void CarveSpaceAudioProcessorEditor::showInfoMenu()
{
    juce::PopupMenu menu;

    juce::PopupMenu themeMenu;
    themeMenu.addItem (100, "Dark", true, currentTheme == ThemeId::dark);
    themeMenu.addItem (101, "Light", true, currentTheme == ThemeId::light);
    themeMenu.addItem (102, "Pink", true, currentTheme == ThemeId::pink);
    menu.addSubMenu ("Theme", themeMenu);

    menu.addSeparator();
    menu.addItem (1, "About CarveSpace");
    menu.addItem (2, "Default size");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&background),
                        [this] (int result)
                        {
                            switch (result)
                            {
                                case 100:
                                    applyTheme (ThemeId::dark);
                                    persistThemeChoice (ThemeId::dark);
                                    break;

                                case 101:
                                    applyTheme (ThemeId::light);
                                    persistThemeChoice (ThemeId::light);
                                    break;

                                case 102:
                                    applyTheme (ThemeId::pink);
                                    persistThemeChoice (ThemeId::pink);
                                    break;

                                case 1:
                                    showAboutDialog();
                                    break;

                                case 2:
                                    if (auto* boundsConstrainer = getConstrainer())
                                        boundsConstrainer->setFixedAspectRatio (1.0);

                                    setSize (designWidth, designHeight);
                                    break;

                                default:
                                    break;
                            }
                        });
}
} // namespace ts
