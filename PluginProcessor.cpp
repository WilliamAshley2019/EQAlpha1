#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

// Required for JUCE plugin instantiation
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Api550aAudioProcessor();
}

Api550aAudioProcessor::Api550aAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo())
        .withOutput("Output", juce::AudioChannelSet::stereo())),
    apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    apvts.addParameterListener("LOW_FREQ", this);
    apvts.addParameterListener("LOW_GAIN", this);
    apvts.addParameterListener("LOW_SHELF", this);
    apvts.addParameterListener("MID_FREQ", this);
    apvts.addParameterListener("MID_GAIN", this);
    apvts.addParameterListener("HIGH_FREQ", this);
    apvts.addParameterListener("HIGH_GAIN", this);
    apvts.addParameterListener("HIGH_SHELF", this);
}

juce::AudioProcessorValueTreeState::ParameterLayout Api550aAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    juce::StringArray freqRange{ "30", "40", "50", "100", "200", "300", "400", "500", "800", "1k", "1.5k", "2k", "3k", "4k", "5k", "8k", "10k", "12k", "16k" };
    juce::StringArray gainRange{ "-12", "-9", "-6", "-3", "0", "3", "6", "9", "12" };

    auto addBand = [&](const juce::String& prefix) {
        params.push_back(std::make_unique<juce::AudioParameterChoice>(prefix + "_FREQ", prefix + " Freq", freqRange, 8));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(prefix + "_GAIN", prefix + " Gain", gainRange, 4));
        };

    addBand("LOW");
    params.push_back(std::make_unique<juce::AudioParameterBool>("LOW_SHELF", "Low Shelf", false));
    addBand("MID");
    addBand("HIGH");
    params.push_back(std::make_unique<juce::AudioParameterBool>("HIGH_SHELF", "High Shelf", false));

    return { params.begin(), params.end() };
}

void Api550aAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec{ sampleRate, static_cast<juce::uint32>(samplesPerBlock), 2 };
    lowFilter.prepare(spec);
    midFilter.prepare(spec);
    highFilter.prepare(spec);
    saturation.prepare(spec);

    saturation.functionToUse = [](float x) {
        const float k = 2.0f; // Saturation drive
        return std::tanh(k * x) / k;
        };

    updateFilters();
}

void Api550aAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    parametersChanged = true;
}

void Api550aAudioProcessor::updateFilters()
{
    parametersChanged = false;
    double sampleRate = getSampleRate();

    auto getChoiceHz = [](const juce::RangedAudioParameter* param) {
        static const float hz[] = { 30, 40, 50, 100, 200, 300, 400, 500, 800, 1000, 1500, 2000, 3000, 4000, 5000, 8000, 10000, 12000, 16000 };
        return hz[static_cast<int>(param->getValue() * (sizeof(hz) / sizeof(hz[0]) - 1))];
        };

    auto getChoiceDb = [](const juce::RangedAudioParameter* param) {
        static const float db[] = { -12, -9, -6, -3, 0, 3, 6, 9, 12 };
        return db[static_cast<int>(param->getValue() * (sizeof(db) / sizeof(db[0]) - 1))];
        };

    // Low Band
    float freq = getChoiceHz(apvts.getParameter("LOW_FREQ"));
    float gain = getChoiceDb(apvts.getParameter("LOW_GAIN"));
    float q = 1.0f + 0.2f * std::abs(gain); // Proportional Q
    bool shelf = *apvts.getRawParameterValue("LOW_SHELF") > 0.5f;
    lowFilter.coefficients = shelf
        ? juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, freq, q, juce::Decibels::decibelsToGain(gain))
        : juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, freq, q, juce::Decibels::decibelsToGain(gain));

    // Mid Band
    freq = getChoiceHz(apvts.getParameter("MID_FREQ"));
    gain = getChoiceDb(apvts.getParameter("MID_GAIN"));
    q = 1.0f + 0.2f * std::abs(gain); // Proportional Q
    midFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, freq, q, juce::Decibels::decibelsToGain(gain));

    // High Band
    freq = getChoiceHz(apvts.getParameter("HIGH_FREQ"));
    gain = getChoiceDb(apvts.getParameter("HIGH_GAIN"));
    q = 1.0f + 0.2f * std::abs(gain); // Proportional Q
    bool highShelf = *apvts.getRawParameterValue("HIGH_SHELF") > 0.5f;
    highFilter.coefficients = highShelf
        ? juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, freq, q, juce::Decibels::decibelsToGain(gain))
        : juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, freq, q, juce::Decibels::decibelsToGain(gain));
}

void Api550aAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    if (parametersChanged)
        updateFilters();

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    lowFilter.process(context);
    midFilter.process(context);
    highFilter.process(context);
    saturation.process(context);
}

void Api550aAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void Api550aAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessorEditor* Api550aAudioProcessor::createEditor()
{
    return new Api550aAudioProcessorEditor(*this);
}