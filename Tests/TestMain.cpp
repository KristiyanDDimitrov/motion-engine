#include <JuceHeader.h>

// This is just a placeholder for now - we'll add actual tests later
class MotionEngineTestRunner : public juce::UnitTestRunner
{
public:
    MotionEngineTestRunner() = default;
    ~MotionEngineTestRunner() override = default;

    void runAllTests()
    {
        // Placeholder for running all tests
        juce::Logger::outputDebugString("MotionEngine test runner initialized");
    }
};

//==============================================================================
class MotionEngineTestsApplication  : public juce::JUCEApplication
{
public:
    MotionEngineTestsApplication() = default;
    ~MotionEngineTestsApplication() override = default;

    void initialise (const juce::String& commandLine) override
    {
        // This method is called when the app starts up.
        // Run tests here
        MotionEngineTestRunner runner;
        runner.runAllTests();

        // Exit with success code
        juce::JUCEApplication::quit();
    }

    void shutdown() override
    {
        // Called when the application shuts down.
    }

    const juce::String getApplicationName() override
    {
        return "MotionEngineTests";
    }

    const juce::String getApplicationVersion() override
    {
        return "1.0.0";
    }

    bool moreThanOneInstanceAllowed() override
    {
        return false;
    }
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION (MotionEngineTestsApplication)