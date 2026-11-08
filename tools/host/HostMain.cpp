/*
    CarveSpaceHost - offline test/reference host for VST3 plugins.

    Commands:
      dump   <plugin.vst3> [--json]
      render <plugin.vst3> --main in.wav [--side sc.wav] --out out.wav
                          [--sr 48000] [--block 512] [--set name=normalised]...
                          [--automate name=normalised@sample]...
                          [--state-in file] [--state-out file]
      shot   <plugin.vst3> --out shot.png [--width 650] [--height 650]
                          [--set name=normalised]...
      state  <plugin.vst3> [--in statefile] [--out statefile]
                          [--set name=normalised]... [--set2 name=normalised]...

    The host allocates a 4-channel process buffer: ch 0-1 main, ch 2-3 sidechain.
    Render output is a 32-bit float stereo WAV; the plugin's reported latency is
    printed on the "wrote ..." line and in "latency=..." lines.
*/

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <map>
#include <vector>
#include <cstdlib>

namespace
{
struct AutomationEvent
{
    juce::String name;
    float value = 0.0f;
    int sample = 0;
    bool applied = false;
};

struct Options
{
    juce::String pluginPath;
    juce::String mainFile;
    juce::String sideFile;
    juce::String outFile;
    juce::String stateFile;    // state command: save destination
    juce::String stateInFile;  // state command / render: load source
    juce::String stateOutFile; // render: save source
    double sampleRate = 48000.0;
    int blockSize = 512;
    int width = 650;
    int height = 650;
    std::map<juce::String, float> paramValues;  // --set
    std::map<juce::String, float> paramValues2; // --set2 (state command scramble)
    std::vector<AutomationEvent> automations;
    bool json = false;
};

void printUsage()
{
    std::cout <<
        "Usage:\n"
        "  CarveSpaceHost dump   <plugin.vst3> [--json]\n"
        "  CarveSpaceHost render <plugin.vst3> --main in.wav [--side sc.wav] --out out.wav\n"
        "                        [--sr 48000] [--block 512] [--set name=normalised]...\n"
        "                        [--automate name=normalised@sample]...\n"
        "                        [--state-in file] [--state-out file]\n"
        "  CarveSpaceHost shot   <plugin.vst3> --out shot.png [--width 650] [--height 650]\n"
        "                        [--set name=normalised]...\n"
        "  CarveSpaceHost state  <plugin.vst3> [--in file] [--out file]\n"
        "                        [--set name=normalised]... [--set2 name=normalised]...\n";
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

juce::AudioProcessorParameter* findParameter (juce::AudioProcessor& plugin, const juce::String& name)
{
    for (auto* parameter : plugin.getParameters())
    {
        const auto id = parameterID (parameter);
        const auto parameterName = parameter->getName (128);

        if (name.equalsIgnoreCase (id) || name.equalsIgnoreCase (parameterName))
            return parameter;
    }

    return nullptr;
}

void applyParameters (juce::AudioProcessor& plugin, const std::map<juce::String, float>& values,
                      std::ostream& out = std::cout)
{
    for (auto& [name, value] : values)
    {
        if (auto* parameter = findParameter (plugin, name))
        {
            parameter->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, value));
            out << "  set id=\"" << parameterID (parameter) << "\" name=\""
                << parameter->getName (128) << "\" normalised=" << value << "\n";
        }
        else
        {
            out << "  !! parameter not found: " << name << "\n";
        }
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

//==============================================================================
juce::String jsonEscape (const juce::String& text)
{
    return text.replace ("\\", "\\\\").replace ("\"", "\\\"");
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

    if (options.json)
    {
        std::cout << "{\n";
        std::cout << "  \"name\": \"" << jsonEscape (plugin->getName()) << "\",\n";
        std::cout << "  \"descriptiveName\": \""
                  << jsonEscape (plugin->getPluginDescription().descriptiveName) << "\",\n";
        std::cout << "  \"manufacturer\": \""
                  << jsonEscape (plugin->getPluginDescription().manufacturerName) << "\",\n";
        std::cout << "  \"version\": \"" << jsonEscape (plugin->getPluginDescription().version) << "\",\n";
        std::cout << "  \"latencySamples\": " << plugin->getLatencySamples() << ",\n";
        std::cout << "  \"tailSamples\": "
                  << (int) (plugin->getTailLengthSeconds() * options.sampleRate) << ",\n";
        std::cout << "  \"inputs\": " << plugin->getTotalNumInputChannels() << ",\n";
        std::cout << "  \"outputs\": " << plugin->getTotalNumOutputChannels() << ",\n";
        std::cout << "  \"inputBuses\": [";

        for (int i = 0; i < plugin->getBusCount (true); ++i)
        {
            if (auto* bus = plugin->getBus (true, i))
            {
                if (i > 0) std::cout << ", ";
                std::cout << "{\"name\": \"" << jsonEscape (bus->getName()) << "\", \"channels\": "
                          << bus->getNumberOfChannels() << ", \"enabled\": "
                          << (bus->isEnabled() ? "true" : "false") << "}";
            }
        }

        std::cout << "],\n  \"outputBuses\": [";

        for (int i = 0; i < plugin->getBusCount (false); ++i)
        {
            if (auto* bus = plugin->getBus (false, i))
            {
                if (i > 0) std::cout << ", ";
                std::cout << "{\"name\": \"" << jsonEscape (bus->getName()) << "\", \"channels\": "
                          << bus->getNumberOfChannels() << ", \"enabled\": "
                          << (bus->isEnabled() ? "true" : "false") << "}";
            }
        }

        std::cout << "],\n  \"parameters\": [\n";

        const auto parameters = plugin->getParameters();

        for (int i = 0; i < parameters.size(); ++i)
        {
            auto* parameter = parameters[i];
            std::cout << "    {\"index\": " << i
                      << ", \"id\": \"" << jsonEscape (parameterID (parameter)) << "\""
                      << ", \"name\": \"" << jsonEscape (parameter->getName (128)) << "\""
                      << ", \"label\": \"" << jsonEscape (parameter->getLabel()) << "\""
                      << ", \"default\": " << parameter->getDefaultValue()
                      << ", \"value\": " << parameter->getValue()
                      << ", \"numSteps\": " << parameter->getNumSteps()
                      << ", \"text\": \"" << jsonEscape (parameter->getText (parameter->getValue(), 64)) << "\"}";
            std::cout << (i + 1 < parameters.size() ? ",\n" : "\n");
        }

        std::cout << "  ]\n}\n";
        return 0;
    }

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

//==============================================================================
bool loadAudioFile (const juce::String& path, juce::AudioBuffer<float>& destination, double& sampleRate)
{
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (juce::File (path)));

    if (reader == nullptr)
        return false;

    sampleRate = reader->sampleRate;
    destination.setSize ((int) juce::jmax ((juce::int64) 1, (juce::int64) reader->numChannels),
                         (int) juce::jmax ((juce::int64) 1, (juce::int64) reader->lengthInSamples));
    destination.clear();
    reader->read (&destination, 0, destination.getNumSamples(), 0, true, true);
    return true;
}

bool saveStateToFile (juce::AudioProcessor& plugin, const juce::String& path, juce::String& error)
{
    juce::MemoryBlock block;
    plugin.getStateInformation (block);

    if (block.getSize() == 0)
    {
        error = "plugin produced an empty state";
        return false;
    }

    juce::File file (path);
    file.deleteFile();

    if (! file.replaceWithData (block.getData(), block.getSize()))
    {
        error = "could not write " + path;
        return false;
    }

    return true;
}

bool loadStateFromFile (juce::AudioProcessor& plugin, const juce::String& path, juce::String& error)
{
    juce::File file (path);

    if (! file.existsAsFile())
    {
        error = "state file does not exist: " + path;
        return false;
    }

    juce::MemoryBlock block;
    file.loadFileAsData (block);

    if (block.getSize() == 0)
    {
        error = "state file is empty: " + path;
        return false;
    }

    plugin.setStateInformation (block.getData(), (int) block.getSize());
    return true;
}

struct RenderStats
{
    int blocks = 0;
    double processMs = 0.0;
    double audioSeconds = 0.0;
};

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

    if (options.stateInFile.isNotEmpty())
    {
        if (! loadStateFromFile (*plugin, options.stateInFile, error))
        {
            std::cerr << "Failed to load state: " << error << "\n";
            return 1;
        }
    }

    if (! prepareBuses (*plugin, mainChannels, sideChannels))
        std::cout << "  (warning: plugin rejected the requested bus layout)\n";

    applyParameters (*plugin, options.paramValues);

    const double sampleRate = options.sampleRate > 0.0 ? options.sampleRate : mainRate;
    plugin->prepareToPlay (sampleRate, options.blockSize);
    const int reportedLatency = plugin->getLatencySamples();

    const int numSamples = mainBuffer.getNumSamples();
    const int totalChannels = mainChannels + sideChannels;
    juce::AudioBuffer<float> processBuffer (juce::jmax (2, totalChannels), options.blockSize);

    juce::AudioBuffer<float> output (2, numSamples);
    output.clear();

    juce::MidiBuffer midi;

    // Automation events are applied in sample order at the first block boundary
    // at or after their requested sample position.
    std::vector<AutomationEvent> automations = options.automations;
    std::sort (automations.begin(), automations.end(),
               [] (const AutomationEvent& a, const AutomationEvent& b) { return a.sample < b.sample; });
    size_t nextAutomation = 0;

    RenderStats stats;
    const auto startTime = std::chrono::steady_clock::now();

    for (int position = 0; position < numSamples; position += options.blockSize)
    {
        while (nextAutomation < automations.size()
               && automations[nextAutomation].sample <= position)
        {
            auto& event = automations[nextAutomation];

            if (auto* parameter = findParameter (*plugin, event.name))
                parameter->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, event.value));
            else
                std::cout << "  !! automation parameter not found: " << event.name << "\n";

            std::cout << "automation id=\"" << event.name << "\" requested_sample="
                      << event.sample << " applied_at=" << position << "\n";
            event.applied = true;
            ++nextAutomation;
        }

        const int samplesThisTime = juce::jmin (options.blockSize, numSamples - position);

        processBuffer.clear();

        for (int channel = 0; channel < mainChannels; ++channel)
            processBuffer.copyFrom (channel, 0, mainBuffer, channel, position, samplesThisTime);

        if (sideChannels > 0)
        {
            const int sideAvailable = juce::jlimit (0, samplesThisTime,
                                                    sideBuffer.getNumSamples() - position);

            if (sideAvailable > 0)
            {
                for (int channel = 0; channel < sideChannels; ++channel)
                    processBuffer.copyFrom (mainChannels + channel, 0, sideBuffer,
                                            juce::jmin (channel, sideBuffer.getNumChannels() - 1),
                                            position, sideAvailable);
            }
        }

        const auto blockStart = std::chrono::steady_clock::now();
        plugin->processBlock (processBuffer, midi);
        const auto blockEnd = std::chrono::steady_clock::now();

        stats.processMs += std::chrono::duration<double, std::milli> (blockEnd - blockStart).count();
        ++stats.blocks;

        for (int channel = 0; channel < 2; ++channel)
            output.copyFrom (channel, position, processBuffer,
                             juce::jmin (channel, mainChannels - 1), 0, samplesThisTime);
    }

    const auto endTime = std::chrono::steady_clock::now();
    stats.audioSeconds = (double) numSamples / sampleRate;

    // Scan the rendered audio for non-finite values and level information.
    double minValue = 0.0, maxValue = 0.0, sumSquares = 0.0;
    juce::int64 nonFinite = 0, totalSamples = 0;

    for (int channel = 0; channel < output.getNumChannels(); ++channel)
    {
        const auto* data = output.getReadPointer (channel);

        for (int i = 0; i < output.getNumSamples(); ++i)
        {
            const auto value = data[i];

            if (! std::isfinite (value))
            {
                ++nonFinite;
                continue;
            }

            minValue = juce::jmin (minValue, (double) value);
            maxValue = juce::jmax (maxValue, (double) value);
            sumSquares += (double) value * (double) value;
            ++totalSamples;
        }
    }

    const double rms = totalSamples > 0 ? std::sqrt (sumSquares / (double) totalSamples) : 0.0;

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

    const auto wallSeconds = std::chrono::duration<double> (endTime - startTime).count();
    const double cpuMsPerBlock = stats.blocks > 0 ? stats.processMs / (double) stats.blocks : 0.0;
    const double realtimeFactor = stats.audioSeconds > 0.0 ? stats.processMs * 0.001 / stats.audioSeconds : 0.0;

    std::cout << "wrote " << options.outFile << " (" << numSamples << " samples, "
              << sampleRate << " Hz, latency " << reportedLatency << " samples)\n";
    std::cout << "latency=" << reportedLatency << "\n";
    std::cout << "render blocks=" << stats.blocks
              << " samples=" << numSamples
              << " cpu_total_ms=" << juce::String (stats.processMs, 3)
              << " cpu_ms_per_block=" << juce::String (cpuMsPerBlock, 4)
              << " realtime_factor=" << juce::String (realtimeFactor, 4)
              << " wall_s=" << juce::String (wallSeconds, 3) << "\n";
    std::cout << "output nonfinite=" << nonFinite
              << " min=" << juce::String (minValue, 6)
              << " max=" << juce::String (maxValue, 6)
              << " rms_db=" << juce::String (juce::Decibels::gainToDecibels ((float) rms, -300.0f), 3)
              << "\n";

    if (options.stateOutFile.isNotEmpty())
    {
        if (saveStateToFile (*plugin, options.stateOutFile, error))
            std::cout << "state saved=" << options.stateOutFile << "\n";
        else
            std::cout << "state save failed: " << error << "\n";
    }

    for (auto& event : automations)
        if (! event.applied)
            std::cout << "automation id=\"" << event.name << "\" requested_sample="
                      << event.sample << " applied_at=never\n";

    return 0;
}

//==============================================================================
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

    juce::DocumentWindow window ("CarveSpaceHost", juce::Colours::black, juce::DocumentWindow::allButtons);
    window.setUsingNativeTitleBar (false);
    window.setContentOwned (editor, false);
    window.setResizable (true, false);
    window.setSize (options.width, options.height);
    editor->setSize (options.width, options.height);
    window.setVisible (true);
    window.toFront (true);

    juce::Timer::callAfterDelay (1200, [] { juce::MessageManager::getInstance()->stopDispatchLoop(); });
    juce::MessageManager::getInstance()->runDispatchLoop();

    // The window manager may have forced a different window size; resize the
    // editor explicitly so the snapshot is at the requested resolution.
    editor->setSize (options.width, options.height);

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

//==============================================================================
void printParameterState (juce::AudioProcessor& plugin, const juce::String& tag)
{
    for (auto* parameter : plugin.getParameters())
        std::cout << tag << " id=\"" << parameterID (parameter) << "\""
                  << " name=\"" << parameter->getName (128) << "\""
                  << " value=" << juce::String (parameter->getValue(), 9)
                  << " text=\"" << parameter->getText (parameter->getValue(), 64) << "\"\n";
}

int commandState (const Options& options)
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

    const auto statePath = options.stateFile.isNotEmpty() ? options.stateFile : options.outFile;

    if (options.stateInFile.isNotEmpty())
    {
        if (! loadStateFromFile (*plugin, options.stateInFile, error))
        {
            std::cerr << "Failed to load state: " << error << "\n";
            return 1;
        }

        std::cout << "state loaded=" << options.stateInFile << "\n";
        printParameterState (*plugin, "loaded");
    }

    if (statePath.isNotEmpty())
    {
        applyParameters (*plugin, options.paramValues);

        if (! saveStateToFile (*plugin, statePath, error))
        {
            std::cerr << "Failed to save state: " << error << "\n";
            return 1;
        }

        std::cout << "state saved=" << statePath << "\n";
        printParameterState (*plugin, "saved");

        if (! options.paramValues2.empty())
        {
            applyParameters (*plugin, options.paramValues2);
            printParameterState (*plugin, "changed");

            if (! loadStateFromFile (*plugin, statePath, error))
            {
                std::cerr << "Failed to reload state: " << error << "\n";
                return 1;
            }

            std::cout << "state restored=" << statePath << "\n";
            printParameterState (*plugin, "restored");
        }
    }
    else if (options.stateInFile.isEmpty() && options.paramValues.empty()
             && options.paramValues2.empty())
    {
        printUsage();
        return 1;
    }
    else if (! options.paramValues2.empty())
    {
        applyParameters (*plugin, options.paramValues);
        printParameterState (*plugin, "changed");
    }

    return 0;
}

//==============================================================================
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

        if (option == "--main")            options.mainFile = next();
        else if (option == "--side")       options.sideFile = next();
        else if (option == "--out")        options.outFile = next();
        else if (option == "--in")         options.stateInFile = next();
        else if (option == "--state-in")   options.stateInFile = next();
        else if (option == "--state-out")  options.stateOutFile = next();
        else if (option == "--sr")         options.sampleRate = next().getDoubleValue();
        else if (option == "--block")      options.blockSize = juce::jmax (1, next().getIntValue());
        else if (option == "--width")      options.width = next().getIntValue();
        else if (option == "--height")     options.height = next().getIntValue();
        else if (option == "--json")       options.json = true;
        else if (option == "--set" || option == "--set2")
        {
            const auto pair = next();
            const auto equals = pair.indexOfChar ('=');

            if (equals > 0)
            {
                auto& target = (option == "--set") ? options.paramValues : options.paramValues2;
                target[pair.substring (0, equals)] = pair.substring (equals + 1).getFloatValue();
            }
        }
        else if (option == "--automate")
        {
            const auto spec = next();
            const auto equals = spec.indexOfChar ('=');
            const auto at = spec.lastIndexOfChar ('@');

            if (equals > 0 && at > equals)
            {
                AutomationEvent event;
                event.name = spec.substring (0, equals);
                event.value = spec.substring (equals + 1, at).getFloatValue();
                event.sample = juce::jmax (0, spec.substring (at + 1).getIntValue());
                options.automations.push_back (event);
            }
            else
            {
                std::cerr << "Invalid --automate value (expected name=normalised@sample): " << spec << "\n";
                return 1;
            }
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
    if (command == "state")  return commandState (options);

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
