#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

//==============================================================================
class MotionEngineAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    MotionEngineAudioProcessorEditor (MotionEngineAudioProcessor&);
    ~MotionEngineAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This is just a simple wrapper to hold the processor
    MotionEngineAudioProcessor& processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MotionEngineAudioProcessorEditor)
};