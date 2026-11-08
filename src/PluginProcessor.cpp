#include "PluginProcessor.h"

#include "PluginEditor.h"

namespace ts
{
namespace
{
    juce::NormalisableRange<float> makeLogRange (float minValue, float maxValue)
    {
        juce::NormalisableRange<float> range (minValue, maxValue);
        range.setSkewForCentre (std::sqrt (minValue * maxValue));
        return range;
    }

    juce::String amountText (float value)
    {
        return juce::String (juce::roundToInt (value));
    }

    juce::String frequencyText (float value)
    {
        if (value >= 1000.0f)
            return juce::String (value / 1000.0f, value >= 10000.0f ? 0 : 1) + " kHz";
        return juce::String (juce::roundToInt (value)) + " Hz";
    }

    juce::String millisecondsText (float value)
    {
        if (value >= 100.0f)
            return juce::String (juce::roundToInt (value)) + " ms";
        if (value >= 10.0f)
            return juce::String (value, 1) + " ms";
        return juce::String (value, 2) + " ms";
    }

    juce::String panText (float value)
    {
        const int rounded = juce::roundToInt (value);
        if (rounded == 0)
            return "Center";
        return (rounded < 0 ? "L " : "R ") + juce::String (std::abs (rounded));
    }
} // namespace

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout TrackspacerAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { params::amount, 1 }, "Amount",
        juce::NormalisableRange<float> (params::amountMin, params::amountMax, 0.01f),
        params::amountDefault,
        juce::AudioParameterFloatAttributes()
            .withLabel ("%")
            .withStringFromValueFunction ([] (float value, int) { return amountText (value); })));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { params::lowCut, 1 }, "Low-cut",
        makeLogRange (params::cutMin, params::cutMax),
        params::lowCutDefault,
        juce::AudioParameterFloatAttributes()
            .withLabel ("Hz")
            .withStringFromValueFunction ([] (float value, int) { return frequencyText (value); })));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { params::highCut, 1 }, "High-cut",
        makeLogRange (params::cutMin, params::cutMax),
        params::highCutDefault,
        juce::AudioParameterFloatAttributes()
            .withLabel ("Hz")
            .withStringFromValueFunction ([] (float value, int) { return frequencyText (value); })));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { params::attack, 1 }, "Attack",
        makeLogRange (params::attackMin, params::attackMax),
        params::attackDefault,
        juce::AudioParameterFloatAttributes()
            .withLabel ("ms")
            .withStringFromValueFunction ([] (float value, int) { return millisecondsText (value); })));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { params::release, 1 }, "Release",
        makeLogRange (params::releaseMin, params::releaseMax),
        params::releaseDefault,
        juce::AudioParameterFloatAttributes()
            .withLabel ("ms")
            .withStringFromValueFunction ([] (float value, int) { return millisecondsText (value); })));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { params::pan, 1 }, "Pan",
        juce::NormalisableRange<float> (params::panMin, params::panMax, 0.01f),
        params::panDefault,
        juce::AudioParameterFloatAttributes()
            .withLabel ("")
            .withStringFromValueFunction ([] (float value, int) { return panText (value); })));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { params::mode, 1 }, "Stereo Mode",
        juce::StringArray { "L/R", "M/S" }, 0));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { params::freeze, 1 }, "Freeze", false));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { params::scAudition, 1 }, "Sidechain", false));

    return layout;
}

//==============================================================================
TrackspacerAudioProcessor::TrackspacerAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withInput ("Sidechain", juce::AudioChannelSet::stereo(), false)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    amountParam = dynamic_cast<juce::AudioParameterFloat*> (apvts.getParameter (params::amount));
    lowCutParam = dynamic_cast<juce::AudioParameterFloat*> (apvts.getParameter (params::lowCut));
    highCutParam = dynamic_cast<juce::AudioParameterFloat*> (apvts.getParameter (params::highCut));
    attackParam = dynamic_cast<juce::AudioParameterFloat*> (apvts.getParameter (params::attack));
    releaseParam = dynamic_cast<juce::AudioParameterFloat*> (apvts.getParameter (params::release));
    panParam = dynamic_cast<juce::AudioParameterFloat*> (apvts.getParameter (params::pan));
    modeParam = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (params::mode));
    freezeParam = dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter (params::freeze));
    scAuditionParam = dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter (params::scAudition));

    jassert (amountParam != nullptr && lowCutParam != nullptr && highCutParam != nullptr);
    jassert (attackParam != nullptr && releaseParam != nullptr && panParam != nullptr);
    jassert (modeParam != nullptr && freezeParam != nullptr && scAuditionParam != nullptr);
}

//==============================================================================
void TrackspacerAudioProcessor::prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock)
{
    engine.prepare (sampleRate);
    sidechainScratch.setSize (2, juce::jmax (maximumExpectedSamplesPerBlock, 1));
    sidechainScratch.clear();
    setLatencySamples (engine.getLatencySamples());
}

void TrackspacerAudioProcessor::releaseResources()
{
    engine.reset();
}

bool TrackspacerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mainIn = layouts.getMainInputChannelSet();
    const auto mainOut = layouts.getMainOutputChannelSet();

    if (mainIn != mainOut)
        return false;

    if (mainIn != juce::AudioChannelSet::mono() && mainIn != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.inputBuses.size() > 1)
    {
        const auto sidechain = layouts.inputBuses[1];

        if (sidechain != juce::AudioChannelSet::disabled()
            && sidechain != juce::AudioChannelSet::mono()
            && sidechain != juce::AudioChannelSet::stereo())
            return false;
    }

    return true;
}

void TrackspacerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midi);

    const int numSamples = buffer.getNumSamples();

    // Make sure any output channels beyond the main output are silent.
    for (int channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear (channel, 0, numSamples);

    // Locate the external sidechain, if the host enabled and connected it.
    juce::AudioBuffer<float> sidechainBuffer;
    bool haveSidechain = false;

    if (getBusCount (true) > 1)
    {
        if (auto* bus = getBus (true, 1))
        {
            const int mainChannels = getMainBusNumInputChannels();
            const int sidechainChannels = bus->getNumberOfChannels();
            const bool bufferHasSidechainChannels = buffer.getNumChannels() >= mainChannels + sidechainChannels;

            if (bus->isEnabled() && bufferHasSidechainChannels)
            {
                sidechainBuffer = getBusBuffer (buffer, true, 1);
                haveSidechain = sidechainBuffer.getNumChannels() > 0
                                && sidechainBuffer.getNumSamples() >= numSamples;
            }
        }
    }

    CarverSettings settings;
    settings.amountPercent = amountParam != nullptr ? amountParam->get() : 0.0f;
    settings.lowCutHz = lowCutParam != nullptr ? lowCutParam->get() : 20.0f;
    settings.highCutHz = highCutParam != nullptr ? highCutParam->get() : 20000.0f;
    settings.attackMs = attackParam != nullptr ? attackParam->get() : 10.0f;
    settings.releaseMs = releaseParam != nullptr ? releaseParam->get() : 100.0f;
    settings.pan = panParam != nullptr ? panParam->get() : 0.0f;
    settings.mode = modeParam != nullptr ? modeParam->getIndex() : 0;
    settings.freeze = freezeParam != nullptr && freezeParam->get();
    settings.scAudition = scAuditionParam != nullptr && scAuditionParam->get();

    // Keep high-cut above low-cut.
    if (settings.highCutHz < settings.lowCutHz * 1.01f)
        settings.highCutHz = juce::jmin (params::cutMax, settings.lowCutHz * 1.01f);

    if (haveSidechain)
    {
        engine.process (buffer, sidechainBuffer, numSamples, settings);
    }
    else
    {
        // Internal sidechain: the input triggers the carver on itself.
        engine.process (buffer, {}, numSamples, settings);
    }
}

//==============================================================================
CarverSettings TrackspacerAudioProcessor::currentSettings() const
{
    CarverSettings settings;
    settings.amountPercent = amountParam != nullptr ? amountParam->get() : 0.0f;
    settings.lowCutHz = lowCutParam != nullptr ? lowCutParam->get() : 20.0f;
    settings.highCutHz = highCutParam != nullptr ? highCutParam->get() : 20000.0f;
    settings.attackMs = attackParam != nullptr ? attackParam->get() : 10.0f;
    settings.releaseMs = releaseParam != nullptr ? releaseParam->get() : 100.0f;
    settings.pan = panParam != nullptr ? panParam->get() : 0.0f;
    settings.mode = modeParam != nullptr ? modeParam->getIndex() : 0;
    settings.freeze = freezeParam != nullptr && freezeParam->get();
    settings.scAudition = scAuditionParam != nullptr && scAuditionParam->get();
    return settings;
}

DisplayFrame TrackspacerAudioProcessor::getDisplayFrame() const
{
    DisplayFrame frame;
    engine.getDisplayFrame (frame);
    return frame;
}

//==============================================================================
void TrackspacerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
    {
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, destData);
    }
}

void TrackspacerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* TrackspacerAudioProcessor::createEditor()
{
    return new TrackspacerAudioProcessorEditor (*this);
}

} // namespace ts

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ts::TrackspacerAudioProcessor();
}
