#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>

#include "Parameters.h"
#include "dsp/CarverEngine.h"

namespace ts
{
class TrackspacerAudioProcessor final : public juce::AudioProcessor
{
public:
    TrackspacerAudioProcessor();
    ~TrackspacerAudioProcessor() override = default;

    //==========================================================================
    void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Trackspacer Clone"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.2; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    CarverSettings currentSettings() const;

    DisplayFrame getDisplayFrame() const;

    bool isAdvancedPanelOpen() const noexcept { return advancedPanelOpen.load(); }
    void setAdvancedPanelOpen (bool shouldBeOpen) noexcept { advancedPanelOpen.store (shouldBeOpen); }

    juce::AudioProcessorValueTreeState apvts;

    juce::AudioParameterFloat* amountParam = nullptr;
    juce::AudioParameterFloat* lowCutParam = nullptr;
    juce::AudioParameterFloat* highCutParam = nullptr;
    juce::AudioParameterFloat* attackParam = nullptr;
    juce::AudioParameterFloat* releaseParam = nullptr;
    juce::AudioParameterFloat* panParam = nullptr;
    juce::AudioParameterChoice* modeParam = nullptr;
    juce::AudioParameterBool* freezeParam = nullptr;
    juce::AudioParameterBool* scAuditionParam = nullptr;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    CarverEngine engine;
    std::atomic<bool> advancedPanelOpen { false };
    juce::AudioBuffer<float> sidechainScratch;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackspacerAudioProcessor)
};

} // namespace ts
