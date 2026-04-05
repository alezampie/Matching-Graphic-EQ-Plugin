#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "PluginParameters.h"
#include "MyTheme.h"
#include "BinaryData.h"

typedef AudioProcessorValueTreeState::SliderAttachment SliderAttachment; //per risparmiare delle righe
typedef AudioProcessorValueTreeState::ButtonAttachment ButtonAttachment;

class GraphicEQAudioProcessorEditor  : public juce::AudioProcessorEditor, private juce::Slider::Listener, private juce::Timer
{
public:
    GraphicEQAudioProcessorEditor (GraphicEQAudioProcessor&, AudioProcessorValueTreeState&, const float*);
    ~GraphicEQAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    //mi creo un metodo setup slider per non ripetere lo stesso codice più volte
    void setupSlider(juce::Slider& slider, int x, int y, int w, int h);

    GraphicEQAudioProcessor& audioProcessor;

    AudioProcessorValueTreeState& valueTreeState;

    //EQ 10 bande
    std::array<juce::Slider, MAX_NUM_BANDS> sliderBand; //creo un array e non slider per slider...
    std::unique_ptr<SliderAttachment> attachmentBand[MAX_NUM_BANDS];

    //global gain
    juce::Slider sliderGlobalGain;
    std::unique_ptr<SliderAttachment> attachmentGlobalGain;

    MyLookAndFeel myTheme;

    //per la curva di EQ
    void drawEQCurve(juce::Graphics& g);
    void sliderValueChanged(juce::Slider* slider) override; //ci servirà per chiamare repaint

    //font personalizzato
    juce::Font basis33Font; //mi piace un sacco

    //frequenze
    const float* centerFrequencies; //puntatore alle freq dal GainMatrixCalculator

    //per il learn target e reference
    juce::TextButton learnTargetButton{ "Learn Target" };
    juce::TextButton learnReferenceButton{ "Learn Reference" };

    //per il match
    juce::TextButton matchButton{ "Match!" };

    //timer per fare repaint ogni tot (utile per cambiare stato dei pulsanti di learn, match e s/c)
    void timerCallback() override;

    //per gestire sidechain
    juce::ToggleButton sidechainToggle;
    std::unique_ptr<ButtonAttachment> sidechainAttachment;
    juce::Label sidechainLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GraphicEQAudioProcessorEditor)
};
