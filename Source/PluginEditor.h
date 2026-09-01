#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class MotionEngineAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    MotionEngineAudioProcessorEditor (juce::AudioProcessor&);
    ~MotionEngineAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to access the processor.
    juce::AudioProcessor& processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MotionEngineAudioProcessorEditor)
};