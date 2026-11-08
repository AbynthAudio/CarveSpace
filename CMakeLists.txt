cmake_minimum_required(VERSION 3.22)

project(TrackspacerClone VERSION 1.0.0 LANGUAGES C CXX)

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
juce_add_plugin(TrackspacerClone
    COMPANY_NAME              "OpenClone Audio"
    BUNDLE_ID                 "com.openclones.trackspacerclone"
    IS_SYNTH                  FALSE
    NEEDS_MIDI_INPUT          FALSE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD   FALSE
    PLUGIN_MANUFACTURER_CODE  Ocau
    PLUGIN_CODE               Trkc
    PRODUCT_NAME              "Trackspacer Clone"
    PLUGIN_VERSION            1.0.0
    VST3_CATEGORIES           Fx EQ
    AU_MAIN_TYPE              kAudioUnitType_Effect
    FORMATS                   VST3 Standalone)

target_sources(TrackspacerClone PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
    src/dsp/CarverEngine.cpp
    src/dsp/BandAnalyzer.cpp
    src/gui/TrackspacerLookAndFeel.cpp
    src/gui/SpectrumDisplay.cpp
    src/gui/AdvancedPanel.cpp
    src/gui/BackgroundComponent.cpp
    src/gui/ParameterKnob.cpp)

target_compile_definitions(TrackspacerClone PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST3=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:TrackspacerClone,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:TrackspacerClone,JUCE_VERSION>")

target_link_libraries(TrackspacerClone PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

target_include_directories(TrackspacerClone PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

juce_generate_juce_header(TrackspacerClone)

# ------------------------------------------------------------------------------
# Offline test / reference host (loads VST3 bundles, dumps parameters, renders)
# ------------------------------------------------------------------------------
juce_add_console_app(TrackspacerHost
    PRODUCT_NAME "TrackspacerHost"
    COMPANY_NAME "OpenClone Audio")

target_sources(TrackspacerHost PRIVATE
    tools/host/HostMain.cpp)

target_compile_definitions(TrackspacerHost PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_STANDALONE_APPLICATION=1)

target_link_libraries(TrackspacerHost PRIVATE
    juce::juce_audio_processors
    juce::juce_audio_formats
    juce::juce_gui_basics
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

target_include_directories(TrackspacerHost PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

juce_generate_juce_header(TrackspacerHost)

# ------------------------------------------------------------------------------
# Convenience: copy the built VST3 bundle into ./dist
# ------------------------------------------------------------------------------
add_custom_target(copy_vst3
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/dist"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_BINARY_DIR}/TrackspacerClone_artefacts/Release/VST3/Trackspacer Clone.vst3"
            "${CMAKE_SOURCE_DIR}/dist/Trackspacer Clone.vst3"
    DEPENDS TrackspacerClone_VST3
    COMMENT "Copying VST3 bundle to dist/")
