cmake_minimum_required(VERSION 3.16)
project(MotionEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE as a submodule
add_subdirectory(JUCE)

# Create the test executable - we need to properly configure this for testing only
add_executable(MotionEngineTests
    Tests/TestMain.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/dsp/EnvelopeFollower.cpp
    Source/dsp/TransientDetector.cpp
    Source/dsp/LFO.cpp
    Source/dsp/ModMatrix.cpp
    Source/dsp/FilterStage.cpp
    Source/dsp/DriveStage.cpp
)

# Link against JUCE components needed for testing
target_link_libraries(MotionEngineTests PRIVATE
    juce::juce_audio_utils
    juce::juce_unit_test
)

# Include directories for tests
target_include_directories(MotionEngineTests PRIVATE
    Source
    Source/dsp
    JUCE/modules
)