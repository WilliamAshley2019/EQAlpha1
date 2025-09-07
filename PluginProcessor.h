#pragma once
#include <JuceHeader.h>

class Api550aAudioProcessor : public juce::AudioProcessor,
    private juce::AudioProcessorValueTreeState::Listener
{
public:
    Api550aAudioProcessor();
    ~Api550aAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Public for editor access
    juce::AudioProcessorValueTreeState apvts;

private:
    // DSP components
    juce::dsp::IIR::Filter<float> lowFilter, midFilter, highFilter;
    juce::dsp::WaveShaper<float> saturation;
    float saturationDrive = 2.0f;

    bool parametersChanged = true;
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void updateFilters();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Api550aAudioProcessor)
};