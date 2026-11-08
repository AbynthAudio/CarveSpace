cmake_minimum_required(VERSION 3.22)

project(CarveSpace VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

add_subdirectory(JUCE)

# ------------------------------------------------------------------------------
# Plugin
# ------------------------------------------------------------------------------
juce_add_plugin(CarveSpace
    COMPANY_NAME              "Abynth Audio"
    BUNDLE_ID                 "com.abynthaudio.carvespace"
    IS_SYNTH                  FALSE
    NEEDS_MIDI_INPUT          FALSE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD   FALSE
    PLUGIN_MANUFACTURER_CODE  Abyn
    PLUGIN_CODE               CrvS
    PRODUCT_NAME              "CarveSpace"
    PLUGIN_VERSION            1.0.0
    VST3_CATEGORIES           Fx EQ
    AU_MAIN_TYPE              kAudioUnitType_Effect
    FORMATS                   VST3 Standalone)

target_sources(CarveSpace PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
    src/dsp/CarverEngine.cpp
    src/dsp/BandAnalyzer.cpp
    src/gui/CarveSpaceLookAndFeel.cpp
    src/gui/SpectrumDisplay.cpp
    src/gui/AdvancedPanel.cpp
    src/gui/BackgroundComponent.cpp
    src/gui/ParameterKnob.cpp)

target_compile_definitions(CarveSpace PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST3=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:CarveSpace,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:CarveSpace,JUCE_VERSION>")

target_link_libraries(CarveSpace PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

target_include_directories(CarveSpace PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

juce_add_binary_data(CarveSpaceAssets SOURCES
    assets/abynth_logo.png
    assets/abynth_icon.png
    assets/abynth_wordmark.png
    assets/fonts/Comfortaa-Bold.ttf)

target_link_libraries(CarveSpace PRIVATE CarveSpaceAssets)

juce_generate_juce_header(CarveSpace)

# ------------------------------------------------------------------------------
# Offline test / reference host (loads VST3 bundles, dumps parameters, renders)
# ------------------------------------------------------------------------------
juce_add_console_app(CarveSpaceHost
    PRODUCT_NAME "CarveSpaceHost"
    COMPANY_NAME "Abynth Audio")

target_sources(CarveSpaceHost PRIVATE
    tools/host/HostMain.cpp)

target_compile_definitions(CarveSpaceHost PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_STANDALONE_APPLICATION=1
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_VST=0
    JUCE_PLUGINHOST_LADSPA=0
    JUCE_PLUGINHOST_LV2=0
    JUCE_PLUGINHOST_ARA=0)

target_link_libraries(CarveSpaceHost PRIVATE
    juce::juce_audio_processors
    juce::juce_audio_formats
    juce::juce_gui_basics
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

target_include_directories(CarveSpaceHost PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

juce_generate_juce_header(CarveSpaceHost)

# ------------------------------------------------------------------------------
# Convenience: copy the built VST3 bundle into ./dist
# ------------------------------------------------------------------------------
add_custom_target(copy_vst3
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/dist"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_BINARY_DIR}/CarveSpace_artefacts/Release/VST3/CarveSpace.vst3"
            "${CMAKE_SOURCE_DIR}/dist/CarveSpace.vst3"
    DEPENDS CarveSpace_VST3
    COMMENT "Copying VST3 bundle to dist/")
