cmake_minimum_required(VERSION 3.22)
project(MotionEngine VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version" FORCE)
endif()

add_subdirectory(JUCE)

# ---------------------------------------------------------------- plugin

juce_add_plugin(MotionEngine
    COMPANY_NAME                "MotionEngine"
    PLUGIN_MANUFACTURER_CODE    Motn
    PLUGIN_CODE                 Mtne
    FORMATS                     VST3
    PRODUCT_NAME                "MotionEngine"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     TRUE)

target_sources(MotionEngine PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_include_directories(MotionEngine PRIVATE Source)

target_compile_definitions(MotionEngine PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(MotionEngine
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_audio_basics
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# PluginProcessor.h / PluginEditor.h #include <JuceHeader.h> — that header is
# not created automatically, it has to be requested per target.
juce_generate_juce_header(MotionEngine)

# ---------------------------------------------------------------- tests
#
# The DSP components under Source/dsp/ are HEADER ONLY. Do not add .cpp
# files here; if a new DSP header appears it is picked up via the include
# directory below with no CMake change required.

juce_add_console_app(MotionEngineTests
    PRODUCT_NAME "MotionEngineTests")

target_sources(MotionEngineTests PRIVATE
    Tests/TestMain.cpp)

target_include_directories(MotionEngineTests PRIVATE Source)

target_compile_definitions(MotionEngineTests PUBLIC
    JUCE_UNIT_TESTS=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_STANDALONE_APPLICATION=1)

target_link_libraries(MotionEngineTests
    PRIVATE
        juce::juce_audio_processors
        juce::juce_audio_basics
        juce::juce_dsp
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# Tests/TestMain.cpp also #include <JuceHeader.h>.
juce_generate_juce_header(MotionEngineTests)
