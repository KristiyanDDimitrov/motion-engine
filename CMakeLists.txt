cmake_minimum_required(VERSION 3.15)
project(MotionEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE as a subdirectory
add_subdirectory(JUCE)

# Create the plugin target
juce_add_plugin(MotionEngine
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE "Mote"
    PLUGIN_PRODUCT_CODE "MEng"
    PLUGIN_NAME "MotionEngine"
    PLUGIN_DESCRIPTION "Audio effect plugin for bass processing"
)

# Add the source files
target_sources(MotionEngine
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

# Add the DSP headers
target_include_directories(MotionEngine
    PRIVATE
        Source
        Source/dsp
)

# Link JUCE
target_link_libraries(MotionEngine PRIVATE juce::juce_audio_utils)

# Create the test target
juce_add_console_app(MotionEngineTests
    PLUGIN_MANUFACTURER_CODE "Mote"
    PLUGIN_PRODUCT_CODE "MEng"
    PLUGIN_NAME "MotionEngineTests"
)

# Add test source files
target_sources(MotionEngineTests
    PRIVATE
        Tests/TestMain.cpp
)

# Include DSP headers for tests
target_include_directories(MotionEngineTests
    PRIVATE
        Source
        Source/dsp
)

# Link JUCE for tests
target_link_libraries(MotionEngineTests PRIVATE juce::juce_audio_utils)