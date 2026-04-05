#pragma once

#include <JuceHeader.h>
#include "AbstractAudioProcessor.h"

#include "PluginParameters.h"
#include "GainMatrixCalculator.h"
#include "FilterBank.h"

class GraphicEQAudioProcessor  : public AbstractAudioProcessor, AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    GraphicEQAudioProcessor();
    ~GraphicEQAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //funzioni getter per vedere se target o reference sono in fase di learn, da usare nell'editor
    bool isLearningTarget() const { return learningTarget.load(); }
    bool isLearningReference() const { return learningReference.load(); }

    //per triggerare learn e match
    void triggerLearnTarget();
    void triggerLearnReference();
    void triggerMatch();


private:
    void parameterChanged(const String& paramID, float newValue) override;
    AudioProcessorValueTreeState parameters;
    float userGain[MAX_NUM_BANDS] = { 0.0f };
    GainMatrixCalculator gmc;
    FilterBank filters;

    //per la parte di learn target e reference
    std::atomic<bool> learningTarget{ false };
    std::atomic<bool> learningReference{ false };

    int learnSamplesTarget = 0;
    int learnSamplesReference = 0;

    int learnLengthSamples = 0;

    std::atomic<bool> hasLearnedTarget{ false };
    std::atomic<bool> hasLearnedReference{ false };

    juce::AudioBuffer<float> learnTargetBuffer;
    juce::AudioBuffer<float> learnReferenceBuffer;
    
    std::atomic<bool> useSidechainAsReference = false;

    void handleLearn(const juce::AudioBuffer<float>& inputBuffer, const juce::AudioBuffer<float>& sidechainBuffer);

    void startLearnTarget(); //funzioni helper da mettere nella parameter changed che modificano i vari valori bool
    void startLearnReference();

    //parte per fare il match
    void handleMatch();
    void normalizeBuffer(juce::AudioBuffer<float>& buffer);
    void generateEqCurve(const std::array<float, MAX_NUM_BANDS>& targetAnalysis, const std::array<float, MAX_NUM_BANDS>& referenceAnalysis, std::array<float, MAX_NUM_BANDS>& eqCurveDb);

    AnalysisFilterBank analysisFilters;

    //per il gain
    float globalGain = 0.0f; //interfaccia in dB
    //sarà questo applicato al buffer (sarà globalGain + G0)
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> totalGainSmoothed;
    void updateTotalGainTarget(); //funzione ausiliaria per il set del target totalGainSmoothed


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GraphicEQAudioProcessor)
};
