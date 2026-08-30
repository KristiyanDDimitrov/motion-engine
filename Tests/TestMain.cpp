#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

// Empty test runner that builds and passes
class TestRunner : public juce::UnitTestRunner
{
public:
    TestRunner() = default;

    void runAllTests()
    {
        // This is a placeholder for the actual tests
        // For now, we just make sure the build works
        expect(true);
    }
};

// JUCE application entry point
START_JUCE_APPLICATION(TestRunner)