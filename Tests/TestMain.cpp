// =============================================================================
// MotionEngine test runner.  HARNESS FILE - DO NOT EDIT.
//
// This file contains ONLY the runner. To add tests, create a NEW file
// Tests/TestSomething.cpp containing a juce::UnitTest subclass and one static
// instance of it. CMake globs Tests/*.cpp, so no build-system change is needed
// and this file never has to be touched.
//
//     struct EnvelopeFollowerTests : public juce::UnitTest
//     {
//         EnvelopeFollowerTests() : juce::UnitTest ("EnvelopeFollower", "dsp") {}
//         void runTest() override
//         {
//             beginTest ("output is never negative");
//             expect (result >= 0.0f, "envelope went negative");
//         }
//     };
//     static EnvelopeFollowerTests envelopeFollowerTests;
//
// The process exits 0 only when at least one test ran and every expect()
// passed. Anything else is a nonzero exit.
// =============================================================================

#include <JuceHeader.h>
#include <iostream>

namespace
{

class ConsoleRunner : public juce::UnitTestRunner
{
public:
    void logMessage (const juce::String& message) override
    {
        std::cout << message.toStdString() << std::endl;
    }
};

// Proves the runner itself executes and can report a result. If this is the
// only test that runs, the suite is empty and the run is treated as a failure.
struct HarnessSelfTest final : public juce::UnitTest
{
    HarnessSelfTest() : juce::UnitTest ("HarnessSelfTest", "harness") {}

    void runTest() override
    {
        beginTest ("runner executes registered tests");
        expect (true, "the runner reached the self test");
    }
};

static HarnessSelfTest harnessSelfTest;

} // namespace

int main (int, char**)
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    const int registered = juce::UnitTest::getAllTests().size();

    ConsoleRunner runner;
    runner.setAssertOnFailure (false);
    runner.setPassesAreLogged (false);
    runner.runAllTests();

    int passes = 0, failures = 0;

    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        if (auto* r = runner.getResult (i))
        {
            passes   += r->passes;
            failures += r->failures;

            for (const auto& m : r->messages)
                std::cout << "  FAIL [" << r->unitTestName.toStdString() << " / "
                          << r->subcategoryName.toStdString() << "] "
                          << m.toStdString() << std::endl;
        }
    }

    std::cout << "\n================ MotionEngine test summary ================\n"
              << "  test classes registered : " << registered << "\n"
              << "  result blocks           : " << runner.getNumResults() << "\n"
              << "  assertions passed       : " << passes << "\n"
              << "  assertions failed       : " << failures << std::endl;

    if (failures > 0)
    {
        std::cout << "  RESULT                  : FAILED\n"
                  << "===========================================================" << std::endl;
        return 1;
    }

    // Only the harness self test is registered -> there is no real coverage yet.
    // Do not let an empty suite masquerade as a passing build.
    if (registered <= 1)
    {
        std::cout << "  RESULT                  : NO TESTS\n"
                  << "  Only the harness self test is registered. Add tests as\n"
                  << "  Tests/Test<Component>.cpp before ticking a DSP task.\n"
                  << "===========================================================" << std::endl;
        return 2;
    }

    std::cout << "  RESULT                  : PASSED\n"
              << "===========================================================" << std::endl;
    return 0;
}
