#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
MotionEngineAudioProcessor::MotionEngineAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties().withMainOutputs (2, juce::AudioChannelSet::stereo()))
#endif
{
}

MotionEngineAudioProcessor::~MotionEngineAudioProcessor()
{
}

//==============================================================================
const juce::String MotionEngineAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MotionEngineAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool MotionEngineAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool MotionEngineAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double MotionEngineAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MotionEngineAudioProcessor::getNumPrograms()
{
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
              // so this should be at least 1, even if you're not really implementing programs.
}

int MotionEngineAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MotionEngineAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String MotionEngineAudioProcessor::getProgramName (int index)
{
    return {};
}

void MotionEngineAudioProcessor::changeProgramName (int index, const juce::String& newName)
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
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void MotionEngineAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data.
    buffer.clear();
}

//==============================================================================
bool MotionEngineAudioProcessor::hasEditor() const
{
    return true; // For a plugin without a GUI, you could return false here.
}

juce::AudioProcessorEditor* MotionEngineAudioProcessor::createEditor()
{
    return new MotionEngineAudioProcessorEditor (*this);
}

//==============================================================================
void MotionEngineAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the MemoryBlock.
    // The values stored here will be restored when the plugin is reloaded.
}

void MotionEngineAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from the MemoryBlock.
    // The values stored here will be restored when the plugin is reloaded.
}

//==============================================================================
// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MotionEngineAudioProcessor();
}