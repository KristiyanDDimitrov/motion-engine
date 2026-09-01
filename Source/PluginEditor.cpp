#include "PluginEditor.h"
#include "PluginProcessor.h"

//==============================================================================
MotionEngineAudioProcessorEditor::MotionEngineAudioProcessorEditor (juce::AudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);
}

MotionEngineAudioProcessorEditor::~MotionEngineAudioProcessorEditor()
{
}

//==============================================================================
void MotionEngineAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("MotionEngine", getLocalBounds(), juce::Justification::centred, 1);
}

void MotionEngineAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // controls in your editor...
}