#include "PluginEditor.h"

// ==============================================================================
// Custom LookAndFeel 
// ==============================================================================
class ApiLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ApiLookAndFeel()
    {
        // Define the color palette
        setColour(juce::Slider::thumbColourId, juce::Colour(0xffe2e2e2)); // Pointer color
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff00529e)); // Blue
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff2b2b2b));   // Knob face
        setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        setColour(juce::TextButton::textColourOnId, juce::Colour(0xff00529e)); // Blue when toggled
        setColour(juce::TextButton::textColourOffId, juce::Colours::lightgrey);
        setColour(juce::TextButton::buttonColourId, juce::Colour(0xff333333));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
        const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<float>(x, y, width, height).reduced(10);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        auto lineW = juce::jmin(4.0f, radius * 0.25f);
        auto arcRadius = radius - lineW * 0.5f;
        auto centre = bounds.getCentre();

        // Draw Knob background
        g.setColour(findColour(juce::Slider::rotarySliderFillColourId));
        g.fillEllipse(bounds);

        // Draw Knob outline
        g.setColour(findColour(juce::Slider::rotarySliderOutlineColourId));
        g.drawEllipse(bounds, lineW);

        // Draw the pointer
        juce::Path p;
        p.startNewSubPath(centre.getX(), centre.getY() - radius * 0.4f);
        p.lineTo(centre.getX(), centre.getY() - radius);
        g.setColour(findColour(juce::Slider::thumbColourId));
        g.strokePath(p, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        p.applyTransform(juce::AffineTransform::rotation(toAngle, centre.getX(), centre.getY()));
        g.strokePath(p, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
};

// ==============================================================================
// Editor Implementation
// ==============================================================================
Api550aAudioProcessorEditor::Api550aAudioProcessorEditor(Api550aAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    laf = std::make_unique<ApiLookAndFeel>();
    setLookAndFeel(laf.get());

    // --- Component Setup ---
    setupSlider(lowFreqSlider);
    setupSlider(lowGainSlider);
    setupSlider(midFreqSlider);
    setupSlider(midGainSlider);
    setupSlider(highFreqSlider);
    setupSlider(highGainSlider);

    auto setupLabel = [&](juce::Label& label, const juce::String& text) {
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(14.0f);
        addAndMakeVisible(label);
        };

    setupLabel(lowBandLabel, "LOW");
    setupLabel(midBandLabel, "MID");
    setupLabel(highBandLabel, "HIGH");

    auto setupButton = [&](juce::TextButton& button, const juce::String& text) {
        addAndMakeVisible(button);
        button.setButtonText(text);
        button.setClickingTogglesState(true);
        };

    setupButton(lowShelfButton, "SHELF");
    setupButton(highShelfButton, "SHELF");

    // --- Attachments ---
    lowFreqAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, "LOW_FREQ", lowFreqSlider);
    lowGainAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, "LOW_GAIN", lowGainSlider);
    lowShelfAttachment = std::make_unique<ButtonAttachment>(audioProcessor.apvts, "LOW_SHELF", lowShelfButton);

    midFreqAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, "MID_FREQ", midFreqSlider);
    midGainAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, "MID_GAIN", midGainSlider);

    highFreqAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, "HIGH_FREQ", highFreqSlider);
    highGainAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, "HIGH_GAIN", highGainSlider);
    highShelfAttachment = std::make_unique<ButtonAttachment>(audioProcessor.apvts, "HIGH_SHELF", highShelfButton);

    setSize(450, 380);
}

Api550aAudioProcessorEditor::~Api550aAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void Api550aAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colour(0xff222222)); // Dark charcoal background

    auto bounds = getLocalBounds();

    // Title
    g.setColour(juce::Colours::lightgrey);
    g.setFont(22.0f);
    g.drawText("EQ Alpha 1", bounds.removeFromTop(40), juce::Justification::centred, true);

    // Draw panels for each band
    auto panelBounds = bounds.reduced(10);
    g.setColour(juce::Colour(0xff1a1a1a)); // Slightly lighter charcoal for panels

    const int numBands = 3;
    const int panelWidth = panelBounds.getWidth() / numBands;

    for (int i = 0; i < numBands; ++i)
    {
        g.fillRoundedRectangle(panelBounds.getX() + i * panelWidth, panelBounds.getY(),
            panelWidth - 5, panelBounds.getHeight(), 10.0f);
    }
}

void Api550aAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(50); // Space for title
    bounds.reduce(15, 15);

    const int numBands = 3;
    const int bandWidth = bounds.getWidth() / numBands;

    auto createColumn = [&](int index) {
        return bounds.withX(bounds.getX() + index * bandWidth).withWidth(bandWidth);
        };

    auto lowBandArea = createColumn(0);
    auto midBandArea = createColumn(1);
    auto highBandArea = createColumn(2);

    auto layoutBand = [&](juce::Rectangle<int> area, juce::Label& bandLabel,
        juce::Slider& freqSlider, juce::Slider& gainSlider, juce::TextButton* shelfButton)
        {
            const int labelHeight = 25;
            const int knobSize = 100;
            const int buttonHeight = 25;
            const int spacing = 10;

            bandLabel.setBounds(area.removeFromTop(labelHeight));
            area.removeFromTop(spacing);

            freqSlider.setBounds(area.removeFromTop(knobSize).reduced(5));

            gainSlider.setBounds(area.removeFromTop(knobSize).reduced(5));

            if (shelfButton)
            {
                area.removeFromTop(spacing);
                shelfButton->setBounds(area.removeFromTop(buttonHeight).reduced(15, 0));
            }
        };

    layoutBand(lowBandArea, lowBandLabel, lowFreqSlider, lowGainSlider, &lowShelfButton);
    layoutBand(midBandArea, midBandLabel, midFreqSlider, midGainSlider, nullptr);
    layoutBand(highBandArea, highBandLabel, highFreqSlider, highGainSlider, &highShelfButton);
}

void Api550aAudioProcessorEditor::setupSlider(juce::Slider& slider)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 80, 20);
    addAndMakeVisible(slider);
}