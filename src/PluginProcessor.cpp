#include "PluginProcessor.h"

#include "PluginEditor.h"

namespace ts
{
namespace
{
    juce::PropertiesFile& getAppProperties()
    {
        juce::PropertiesFile::Options options;
        options.applicationName = "CarveSpace";
        options.folderName = ".config/AbynthAudio";
        options.filenameSuffix = "conf";
        options.osxLibrarySubFolder = "Application Support";
        options.storageFormat = juce::PropertiesFile::storeAsXML;

        static juce::PropertiesFile properties (options);
        return properties;
    }

    juce::NormalisableRange<float> makeLogRange (float minValue, float maxValue)
    {
        // Exact logarithmic mapping: value = min * (max/min)^normalised.
        return { minValue, maxValue,
                 [] (float start, float end, float normalised)
                 {
                     return start * std::pow (end / start, normalised);
                 },
                 [] (float start, float end, float value)
                 {
                     return std::log (value / start) / std::log (end / start);
                 },
                 [] (float start, float end, float value)
                 {
                     return juce::jlimit (start, end, value);
                 } };
    }

    juce::String amountText (float value)
    {
        return juce::String (juce::roundToInt (value));
    }

    juce::String frequencyText (float value)
    {
        if (value >= 1000.0f)
            return juce::String (value / 1000.0f, 2) + "k";
        return juce::String (value, 2);
    }

    juce::String millisecondsText (float value)
    {
        return juce::String (value, 2);
    }

    juce::String panText (float value)
    {
        return juce::String (value, 1);
    }

    class SwitchBoolParameter : public juce::AudioParameterBool
    {
    public:
        SwitchBoolParameter (const juce::ParameterID& parameterID, const juce::String& name, bool defaultValue)
            : juce::AudioParameterBool (parameterID, name, defaultValue)
        {
        }

        juce::String getText (float value, int) const override
        {
            return value > 0.5f ? "ON" : "OFF";
        }
    };
} // namespace

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout CarveSpaceAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Order and names match the reference plug-in's host-visible parameter list.
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { params::amount, 1 }, "Ratio",
        juce::NormalisableRange<float> (params::amountMin, params::amountMax, 0.01f),
        params::amountDefault,
        juce::AudioParameterFloatAttributes()
            .withLabel ("%")
            .withStringFromValueFunction ([] (float value, int) { return juce::String (value, 1); })));

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

    layout.add (std::make_unique<SwitchBoolParameter> (
        juce::ParameterID { params::freeze, 1 }, "Freeze", false));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { params::pan, 1 }, "Pan",
        juce::NormalisableRange<float> (params::panMin, params::panMax, 0.01f),
        params::panDefault,
        juce::AudioParameterFloatAttributes()
            .withLabel ("%")
            .withStringFromValueFunction ([] (float value, int) { return juce::String (value, 1); })));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { params::mode, 1 }, "Stereo Mode",
        juce::StringArray { "L/R", "M/S" }, 0));

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

    layout.add (std::make_unique<SwitchBoolParameter> (
        juce::ParameterID { params::scAudition, 1 }, "Sidechain", false));

    return layout;
}

//==============================================================================
CarveSpaceAudioProcessor::CarveSpaceAudioProcessor()
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

    // The engine's latency is fixed by the FFT size. Announce it immediately so
    // that hosts reading latency during their prepareToPlay() see the real value.
    setLatencySamples (engine.getLatencySamples());
}

//==============================================================================
void CarveSpaceAudioProcessor::prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock)
{
    engine.prepare (sampleRate);
    sidechainScratch.setSize (2, juce::jmax (maximumExpectedSamplesPerBlock, 1));
    sidechainScratch.clear();
    setLatencySamples (engine.getLatencySamples());
}

void CarveSpaceAudioProcessor::releaseResources()
{
    engine.reset();
}

bool CarveSpaceAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void CarveSpaceAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
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
CarverSettings CarveSpaceAudioProcessor::currentSettings() const
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

juce::String CarveSpaceAudioProcessor::getUiTheme() const
{
    if (apvts.state.hasProperty ("uiTheme"))
        return apvts.state.getProperty ("uiTheme").toString();

    return getAppProperties().getValue ("theme", "dark");
}

void CarveSpaceAudioProcessor::setUiTheme (const juce::String& themeName)
{
    apvts.state.setProperty ("uiTheme", themeName, nullptr);
    auto& properties = getAppProperties();
    properties.setValue ("theme", themeName);
    properties.saveIfNeeded();
}

int CarveSpaceAudioProcessor::getSavedEditorWidth() const
{
    if (apvts.state.hasProperty ("editorWidth"))
        return (int) apvts.state.getProperty ("editorWidth");

    return (int) getAppProperties().getIntValue ("editorWidth", 650);
}

int CarveSpaceAudioProcessor::getSavedEditorHeight() const
{
    if (apvts.state.hasProperty ("editorHeight"))
        return (int) apvts.state.getProperty ("editorHeight");

    return (int) getAppProperties().getIntValue ("editorHeight", 650);
}

void CarveSpaceAudioProcessor::saveEditorSize (int width, int height)
{
    apvts.state.setProperty ("editorWidth", width, nullptr);
    apvts.state.setProperty ("editorHeight", height, nullptr);

    auto& properties = getAppProperties();
    properties.setValue ("editorWidth", width);
    properties.setValue ("editorHeight", height);
    properties.saveIfNeeded();
}

DisplayFrame CarveSpaceAudioProcessor::getDisplayFrame() const
{
    DisplayFrame frame;
    engine.getDisplayFrame (frame);
    return frame;
}

//==============================================================================
void CarveSpaceAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
    {
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, destData);
    }
}

void CarveSpaceAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* CarveSpaceAudioProcessor::createEditor()
{
    return new CarveSpaceAudioProcessorEditor (*this);
}

} // namespace ts

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ts::CarveSpaceAudioProcessor();
}
