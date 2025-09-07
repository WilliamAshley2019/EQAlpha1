#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// Forward-declare our custom LookAndFeel class
class ApiLookAndFeel;

class Api550aAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    Api550aAudioProcessorEditor(Api550aAudioProcessor&);
    ~Api550aAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    Api550aAudioProcessor& audioProcessor;

    // GUI Components
    juce::Slider lowFreqSlider, midFreqSlider, highFreqSlider;
    juce::Slider lowGainSlider, midGainSlider, highGainSlider;
    juce::TextButton lowShelfButton, highShelfButton;
    juce::Label lowBandLabel, midBandLabel, highBandLabel;

    // Attachments to connect GUI to the processor state
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> lowFreqAttachment, lowGainAttachment;
    std::unique_ptr<SliderAttachment> midFreqAttachment, midGainAttachment;
    std::unique_ptr<SliderAttachment> highFreqAttachment, highGainAttachment;
    std::unique_ptr<ButtonAttachment> lowShelfAttachment, highShelfAttachment;

    // Custom LookAndFeel
    std::unique_ptr<ApiLookAndFeel> laf;

    void setupSlider(juce::Slider& slider);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Api550aAudioProcessorEditor)
};