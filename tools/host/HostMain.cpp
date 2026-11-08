/*
    TrackspacerHost - offline test/reference host for VST3 plugins.

    Commands:
      dump   <plugin.vst3>
      render <plugin.vst3> --main in.wav [--side sc.wav] --out out.wav
                          [--sr 48000] [--block 512] [--set name=normalised]...
      shot   <plugin.vst3> --out shot.png [--width 650] [--height 650]
                          [--set name=normalised]...
*/

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <iostream>
#include <map>

namespace
{
struct Options
{
    juce::String pluginPath;
    juce::String mainFile;
    juce::String sideFile;
    juce::String outFile;
    double sampleRate = 48000.0;
    int blockSize = 512;
    int width = 650;
    int height = 650;
    std::map<juce::String, float> paramValues; // name/id -> normalised value
};

void printUsage()
{
    std::cout << "Usage:\n"
                 "  TrackspacerHost dump   <plugin.vst3>\n"
                 "  TrackspacerHost render <plugin.vst3> --main in.wav [--side sc.wav] --out out.wav"
                 " [--sr 48000] [--block 512] [--set name=normalised]...\n"
                 "  TrackspacerHost shot   <plugin.vst3> --out shot.png [--width 650] [--height 650]"
                 " [--set name=normalised]...\n";
}

std::unique_ptr<juce::AudioPluginInstance> loadPlugin (const juce::String& path,
                                                       juce::AudioPluginFormatManager& formats,
                                                       juce::String& error)
{
    juce::OwnedArray<juce::PluginDescription> descriptions;

    for (auto* format : formats.getFormats())
    {
        if (format->fileMightContainThisPluginType (path))
        {
            format->findAllTypesForFile (descriptions, path);
            if (! descriptions.isEmpty())
                break;
        }
    }

    if (descriptions.isEmpty())
    {
        error = "No plugin types found in " + path;
        return nullptr;
    }

    auto instance = formats.createPluginInstance (*descriptions[0], 48000.0, 512, error);

    if (instance == nullptr)
        return nullptr;

    return instance;
}

juce::String parameterID (juce::AudioProcessorParameter* parameter)
{
    if (auto* hosted = dynamic_cast<juce::HostedAudioProcessorParameter*> (parameter))
        return hosted->getParameterID();

    return {};
}

void applyParameters (juce::AudioProcessor& plugin, const std::map<juce::String, float>& values)
{
    for (auto& [name, value] : values)
    {
        juce::AudioProcessorParameter* match = nullptr;

        for (auto* parameter : plugin.getParameters())
        {
            const auto id = parameterID (parameter);
            const auto parameterName = parameter->getName (128);

            if (name.equalsIgnoreCase (id) || name.equalsIgnoreCase (parameterName))
            {
                match = parameter;
                break;
            }
        }

        if (match != nullptr)
            match->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, value));
        else
            std::cout << "  !! parameter not found: " << name << "\n";
    }
}

bool prepareBuses (juce::AudioProcessor& plugin, int mainChannels, int sidechainChannels)
{
    juce::AudioProcessor::BusesLayout layout;

    if (mainChannels == 1)
        layout.inputBuses.add (juce::AudioChannelSet::mono());
    else
        layout.inputBuses.add (juce::AudioChannelSet::stereo());

    if (sidechainChannels == 1)
        layout.inputBuses.add (juce::AudioChannelSet::mono());
    else if (sidechainChannels == 2)
        layout.inputBuses.add (juce::AudioChannelSet::stereo());
    else
        layout.inputBuses.add (juce::AudioChannelSet::disabled());

    if (mainChannels == 1)
        layout.outputBuses.add (juce::AudioChannelSet::mono());
    else
        layout.outputBuses.add (juce::AudioChannelSet::stereo());

    return plugin.setBusesLayout (layout);
}

int commandDump (const Options& options)
{
    juce::AudioPluginFormatManager formats;
    formats.addDefaultFormats();

    juce::String error;
    auto plugin = loadPlugin (options.pluginPath, formats, error);

    if (plugin == nullptr)
    {
        std::cerr << "Failed to load plugin: " << error << "\n";
        return 1;
    }

    plugin->enableAllBuses();
    plugin->prepareToPlay (options.sampleRate, options.blockSize);

    std::cout << "name: " << plugin->getName() << "\n";
    std::cout << "descriptiveName: " << plugin->getPluginDescription().descriptiveName << "\n";
    std::cout << "manufacturer: " << plugin->getPluginDescription().manufacturerName << "\n";
    std::cout << "version: " << plugin->getPluginDescription().version << "\n";
    std::cout << "latencySamples: " << plugin->getLatencySamples() << "\n";
    std::cout << "tailSamples: " << (int) (plugin->getTailLengthSeconds() * options.sampleRate) << "\n";
    std::cout << "inputs: " << plugin->getTotalNumInputChannels() << "\n";
    std::cout << "outputs: " << plugin->getTotalNumOutputChannels() << "\n";

    for (int dir = 0; dir < 2; ++dir)
    {
        const bool isInput = (dir == 0);
        std::cout << (isInput ? "inputBuses: " : "outputBuses: ") << plugin->getBusCount (isInput) << "\n";

        for (int i = 0; i < plugin->getBusCount (isInput); ++i)
        {
            if (auto* bus = plugin->getBus (isInput, i))
                std::cout << "  bus " << i << " name=\"" << bus->getName() << "\" channels="
                          << bus->getNumberOfChannels() << " enabled=" << (bus->isEnabled() ? "yes" : "no")
                          << " layout=" << bus->getCurrentLayout().getDescription() << "\n";
        }
    }

    const auto parameters = plugin->getParameters();

    for (int i = 0; i < parameters.size(); ++i)
    {
        auto* parameter = parameters[i];

        std::cout << "param " << i
                  << " id=\"" << parameterID (parameter) << "\""
                  << " name=\"" << parameter->getName (128) << "\""
                  << " label=\"" << parameter->getLabel() << "\""
                  << " default=" << parameter->getDefaultValue()
                  << " numSteps=" << parameter->getNumSteps()
                  << " value=" << parameter->getValue()
                  << " text=\"" << parameter->getText (parameter->getValue(), 64) << "\"";

        for (float normalised : { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f })
            std::cout << " [" << normalised << "=" << parameter->getText (normalised, 64) << "]";

        std::cout << "\n";
    }

    return 0;
}

bool loadAudioFile (const juce::String& path, juce::AudioBuffer<float>& destination, double& sampleRate)
{
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (juce::File (path)));

    if (reader == nullptr)
        return false;

    sampleRate = reader->sampleRate;
    destination.setSize ((int) juce::jmax ((juce::int64) 1, (juce::int64) reader->numChannels),
                         (int) juce::jmax ((juce::int64) 1, reader->lengthInSamples));
    destination.clear();
    reader->read (&destination, 0, destination.getNumSamples(), 0, true, true);
    return true;
}

int commandRender (const Options& options)
{
    if (options.mainFile.isEmpty() || options.outFile.isEmpty())
    {
        printUsage();
        return 1;
    }

    juce::AudioPluginFormatManager formats;
    formats.addDefaultFormats();

    juce::String error;
    auto plugin = loadPlugin (options.pluginPath, formats, error);

    if (plugin == nullptr)
    {
        std::cerr << "Failed to load plugin: " << error << "\n";
        return 1;
    }

    juce::AudioBuffer<float> mainBuffer;
    double mainRate = 0.0;

    if (! loadAudioFile (options.mainFile, mainBuffer, mainRate))
    {
        std::cerr << "Failed to read " << options.mainFile << "\n";
        return 1;
    }

    juce::AudioBuffer<float> sideBuffer;

    if (options.sideFile.isNotEmpty())
    {
        double sideRate = 0.0;
        if (! loadAudioFile (options.sideFile, sideBuffer, sideRate))
        {
            std::cerr << "Failed to read " << options.sideFile << "\n";
            return 1;
        }
    }

    const int mainChannels = juce::jmin (2, mainBuffer.getNumChannels());
    const int sideChannels = sideBuffer.getNumChannels() > 0 ? juce::jmin (2, sideBuffer.getNumChannels()) : 0;

    if (! prepareBuses (*plugin, mainChannels, sideChannels))
        std::cout << "  (warning: plugin rejected the requested bus layout)\n";

    applyParameters (*plugin, options.paramValues);

    const double sampleRate = options.sampleRate > 0.0 ? options.sampleRate : mainRate;
    plugin->prepareToPlay (sampleRate, options.blockSize);

    const int numSamples = mainBuffer.getNumSamples();
    const int totalChannels = mainChannels + sideChannels;
    juce::AudioBuffer<float> processBuffer (juce::jmax (2, totalChannels), options.blockSize);

    juce::AudioBuffer<float> output (2, numSamples);
    output.clear();

    juce::MidiBuffer midi;

    for (int position = 0; position < numSamples; position += options.blockSize)
    {
        const int samplesThisTime = juce::jmin (options.blockSize, numSamples - position);

        processBuffer.clear();

        for (int channel = 0; channel < mainChannels; ++channel)
            processBuffer.copyFrom (channel, 0, mainBuffer, channel, position, samplesThisTime);

        for (int channel = 0; channel < sideChannels; ++channel)
            processBuffer.copyFrom (mainChannels + channel, 0, sideBuffer,
                                    juce::jmin (channel, sideBuffer.getNumChannels() - 1),
                                    position, juce::jmin (samplesThisTime, sideBuffer.getNumSamples() - position));

        plugin->processBlock (processBuffer, midi);

        for (int channel = 0; channel < 2; ++channel)
            output.copyFrom (channel, position, processBuffer,
                             juce::jmin (channel, mainChannels - 1), 0, samplesThisTime);
    }

    juce::WavAudioFormat wavFormat;
    auto file = juce::File (options.outFile);
    file.deleteFile();

    std::unique_ptr<juce::FileOutputStream> stream (file.createOutputStream());

    if (stream == nullptr)
    {
        std::cerr << "Failed to create " << options.outFile << "\n";
        return 1;
    }

    std::unique_ptr<juce::AudioFormatWriter> writer (
        wavFormat.createWriterFor (stream.get(), sampleRate, 2, 32, {}, 0));

    if (writer == nullptr)
    {
        std::cerr << "Failed to create WAV writer\n";
        return 1;
    }

    stream.release();
    writer->writeFromAudioSampleBuffer (output, 0, output.getNumSamples());

    std::cout << "wrote " << options.outFile << " (" << numSamples << " samples, "
              << sampleRate << " Hz, latency " << plugin->getLatencySamples() << " samples)\n";

    return 0;
}

int commandShot (const Options& options)
{
    if (options.outFile.isEmpty())
    {
        printUsage();
        return 1;
    }

    juce::AudioPluginFormatManager formats;
    formats.addDefaultFormats();

    juce::String error;
    auto plugin = loadPlugin (options.pluginPath, formats, error);

    if (plugin == nullptr)
    {
        std::cerr << "Failed to load plugin: " << error << "\n";
        return 1;
    }

    applyParameters (*plugin, options.paramValues);
    plugin->prepareToPlay (options.sampleRate, options.blockSize);
    plugin->enableAllBuses();

    auto* editor = plugin->createEditorIfNeeded();

    if (editor == nullptr)
    {
        std::cerr << "Plugin did not create an editor\n";
        return 1;
    }

    juce::DocumentWindow window ("TrackspacerHost", juce::Colours::black, juce::DocumentWindow::allButtons);
    window.setUsingNativeTitleBar (false);
    window.setContentOwned (editor, false);
    window.setSize (options.width, options.height);
    window.setVisible (true);
    window.toFront (true);

    juce::Timer::callAfterDelay (1200, [] { juce::MessageManager::getInstance()->stopDispatchLoop(); });
    juce::MessageManager::getInstance()->runDispatchLoop();

    auto snapshot = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);

    if (! snapshot.isValid())
    {
        std::cerr << "Failed to create snapshot\n";
        return 1;
    }

    juce::File file (options.outFile);
    file.deleteFile();
    std::unique_ptr<juce::FileOutputStream> stream (file.createOutputStream());

    if (stream == nullptr)
    {
        std::cerr << "Failed to create " << options.outFile << "\n";
        return 1;
    }

    juce::PNGImageFormat png;
    if (! png.writeImageToStream (snapshot, *stream))
    {
        std::cerr << "Failed to write PNG\n";
        return 1;
    }

    std::cout << "wrote " << options.outFile << " (" << snapshot.getWidth() << "x" << snapshot.getHeight() << ")\n";
    return 0;
}

int run (int argc, char** argv)
{
    if (argc < 3)
    {
        printUsage();
        return 1;
    }

    const juce::String command (argv[1]);

    Options options;
    options.pluginPath = juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]).getFullPathName();

    for (int i = 3; i < argc; ++i)
    {
        const juce::String option (argv[i]);

        auto next = [&]() -> juce::String
        {
            return (i + 1 < argc) ? juce::String (argv[++i]) : juce::String();
        };

        if (option == "--main")           options.mainFile = next();
        else if (option == "--side")      options.sideFile = next();
        else if (option == "--out")       options.outFile = next();
        else if (option == "--sr")        options.sampleRate = next().getDoubleValue();
        else if (option == "--block")     options.blockSize = next().getIntValue();
        else if (option == "--width")     options.width = next().getIntValue();
        else if (option == "--height")    options.height = next().getIntValue();
        else if (option == "--set")
        {
            const auto pair = next();
            const auto equals = pair.indexOfChar ('=');

            if (equals > 0)
                options.paramValues[pair.substring (0, equals)] = pair.substring (equals + 1).getFloatValue();
        }
        else
        {
            std::cerr << "Unknown option: " << option << "\n";
            printUsage();
            return 1;
        }
    }

    if (command == "dump")   return commandDump (options);
    if (command == "render") return commandRender (options);
    if (command == "shot")   return commandShot (options);

    printUsage();
    return 1;
}
} // namespace

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI initialiser;
    const int result = run (argc, argv);
    return result;
}
