#pragma once

#include <JuceHeader.h>

class AbstractAudioProcessor : public juce::AudioProcessor
{
public:
    AbstractAudioProcessor()
        : juce::AudioProcessor(
            juce::AudioProcessor::BusesProperties()
            .withInput("Input", juce::AudioChannelSet::stereo(), true)
            .withInput("Sidechain", juce::AudioChannelSet::stereo(), true)
            .withOutput("Output", juce::AudioChannelSet::stereo(), true)
        )
    {
    }

    ~AbstractAudioProcessor() override = default;

    const juce::String getName() const override
    {
        return JucePlugin_Name;
    }

    bool acceptsMidi() const override
    {
       #if JucePlugin_WantsMidiInput
        return true;
       #else
        return false;
       #endif
    }

    bool producesMidi() const override
    {
       #if JucePlugin_ProducesMidiOutput
        return true;
       #else
        return false;
       #endif
    }

    bool isMidiEffect() const override
    {
       #if JucePlugin_IsMidiEffect
        return true;
       #else
        return false;
       #endif
    }

    double getTailLengthSeconds() const override
    {
        return 0.0;
    }

    int getNumPrograms() override
    {
        return 1;
    }

    int getCurrentProgram() override
    {
        return 0;
    }

    void setCurrentProgram(int) override
    {
    }

    const juce::String getProgramName(int) override
    {
        return {};
    }

    void changeProgramName(int, const juce::String&) override
    {
    }

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override
    {
        if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
            return false;

        if (layouts.getMainInputChannelSet() != layouts.getMainOutputChannelSet())
            return false;

        //se sidechain attivo, può essere mono o stereo
        auto sidechain = layouts.getChannelSet(true, 1);

        if (!sidechain.isDisabled())
            if (sidechain != juce::AudioChannelSet::mono() &&
                sidechain != juce::AudioChannelSet::stereo())
                return false;

        return true;
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AbstractAudioProcessor)
};


