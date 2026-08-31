#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
MotionEngineAudioProcessor::MotionEngineAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                       .withMainInputChannels (juce::AudioChannelSet::stereo())
                       .withMainOutputChannels (juce::AudioChannelSet::stereo())
                       )
#endif
{
}

MotionEngineAudioProcessor::~MotionEngineAudioProcessor()
{
}

//==============================================================================
void MotionEngineAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void MotionEngineAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool MotionEngineAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // This is the place where you check if the layout is supported.
    // In this case we only support stereo, so we just need to make sure that
    // both input and output have the same number of channels.
    return layouts.getMainInputBuses().size() == 1 && layouts.getMainOutputBuses().size() == 1;
}

void MotionEngineAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);

    // In case we have more outputs than inputs, this code should clear any output
    // channels that didn't contain input data, (because these aren't guaranteed to
    // be empty - they may contain garbage).
    for (int i = getMainOutputChannelSize(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Add your signal processing code here...
}

//==============================================================================
juce::AudioProcessorEditor* MotionEngineAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

bool MotionEngineAudioProcessor::hasEditor() const
{
    return true; // For a real plugin, this should be true
}

//==============================================================================
const juce::String MotionEngineAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MotionEngineAudioProcessor::acceptsMidi() const
{
    return false;
}

bool MotionEngineAudioProcessor::producesMidi() const
{
    return false;
}

bool MotionEngineAudioProcessor::isMidiEffect() const
{
    return false;
}

double MotionEngineAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

//==============================================================================
int MotionEngineAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int MotionEngineAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MotionEngineAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused(index);
}

const juce::String MotionEngineAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused(index);
    return {};
}

void MotionEngineAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void MotionEngineAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the MemoryBlock.
    // The values stored here will be restored when the plugin is reloaded.
    juce::ignoreUnused(destData);
}

void MotionEngineAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents were previously obtained from getStateInformation().
    juce::ignoreUnused(data, sizeInBytes);
}