#include "PluginEditor.h"

PluginEditor::PluginEditor (juce::AudioProcessor& p)
    : AudioProcessorEditor (&p)
{
    setSize (400, 300);
}

PluginEditor::~PluginEditor()
{
}

void PluginEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void PluginEditor::resized()
{
}