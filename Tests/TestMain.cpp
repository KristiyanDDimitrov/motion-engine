#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_unit_test/juce_unit_test.h>

//==============================================================================
class EmptyTest : public juce::UnitTest
{
public:
    EmptyTest() : juce::UnitTest ("Empty Test") {}

    void runTest() override
    {
        // This is just a placeholder test to verify the testing framework works
        expect (true);
    }
};

//==============================================================================
JUCE_BEGIN_IGNORE_WARNINGS_GCC_AND_CLANG (-Wdeprecated-declarations)
JUCE_END_IGNORE_WARNINGS_GCC_AND_CLANG

//==============================================================================
JUCE_IMPLEMENT_UNIT_TEST (EmptyTest)

//==============================================================================
int main()
{
    // Run the test suite
    juce::UnitTestRunner runner;
    runner.runAllTests();

    return 0;
}