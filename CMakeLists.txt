cmake_minimum_required(VERSION 3.16)
project(MotionEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE as a submodule
add_subdirectory(JUCE)

# Create the plugin target
juce_add_plugin(MotionEngine
    COMPANY_NAME "MotionEngine"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE "MEng"
    PLUGIN_CODE "MEng"
    PLUGIN_NAME "MotionEngine"
    PLUGIN_DESCRIPTION "Motion Engine Audio Plugin"
    PLUGIN_VERSION "1.0.0"
    PLUGIN_FORM_FACTOR juce::PluginFormFactor::Normal
    PLUGIN_WINDOW_SIZE 600 400
)

# Add source files for the plugin
target_sources(MotionEngine
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

# Add the DSP headers
target_include_directories(MotionEngine PRIVATE
    Source
    Source/dsp
)

# Link JUCE libraries
target_link_libraries(MotionEngine PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
)

# Create the test executable
juce_add_console_application(MotionEngineTests
    PLUGIN_MANUFACTURER_CODE "MEng"
    PLUGIN_CODE "MEng"
    PLUGIN_NAME "MotionEngineTests"
)

# Add test sources
target_sources(MotionEngineTests
    PRIVATE
        Tests/TestMain.cpp
        Source/dsp/EnvelopeFollower.cpp
        Source/dsp/TransientDetector.cpp
        Source/dsp/LFO.cpp
        Source/dsp/ModMatrix.cpp
        Source/dsp/FilterStage.cpp
        Source/dsp/DriveStage.cpp
)

# Add include directories for tests
target_include_directories(MotionEngineTests PRIVATE
    Source
    Source/dsp
    JUCE/modules
)

# Link JUCE libraries for tests
target_link_libraries(MotionEngineTests PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_audio_basics
)