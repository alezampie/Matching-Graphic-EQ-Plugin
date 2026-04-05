#pragma once
#include <JuceHeader.h>
#include "PluginParameters.h"

#define MAX_NUM_CH 2
#define MAX_NUM_CH_ANALYSIS 1

class FilterBank {
public:
    FilterBank() {
        for (int band = 0; band < MAX_NUM_BANDS; ++band) {
            for (int ch = 0; ch < MAX_NUM_CH; ++ch) {
                filters[band].add(new dsp::IIR::Filter<float>());
            }
        }
    }

    ~FilterBank() {}

    void prepareToPlay(double sr, const float* fc) {
        sampleRate = sr;

        for (int i = 0; i < MAX_NUM_BANDS; ++i) {
            centerFrequencies[i] = fc[i];
            
            gains[i] = 0.0f;
        }

        reset();
        updateCoefficients();
    }

    void processBlock(AudioBuffer<float>& buffer, int numSamples, int numCh) {
        dsp::AudioBlock<float> block(buffer.getArrayOfWritePointers(), numCh, numSamples);

        for (int band = 0; band < MAX_NUM_BANDS; ++band) {
            for (int ch = 0; ch < jmin(numCh, MAX_NUM_CH); ++ch) {
                dsp::AudioBlock<float> chBlock = block.getSingleChannelBlock(ch);
                dsp::ProcessContextReplacing<float> context(chBlock);
                filters[band].getUnchecked(ch)->process(context);
            }
        }
    }

    void setGains(const float* newCommandGain) {
        for (int i = 0; i < MAX_NUM_BANDS; ++i) {
            gains[i] = newCommandGain[i];
        }
        updateCoefficients();
    }

    void reset() {
        for (int band = 0; band < MAX_NUM_BANDS; ++band)
            for (int ch = 0; ch < MAX_NUM_CH; ++ch)
                filters[band].getUnchecked(ch)->reset();
    }

private:
    void updateCoefficients() {
        for (int band = 0; band < MAX_NUM_BANDS; ++band) {

            dsp::IIR::Coefficients<float>::Ptr coeffs;

            const auto fc = centerFrequencies[band];
            const auto gainDb = gains[band];
            const auto g = Decibels::decibelsToGain(gainDb);

            if (band == 0) {
                //low shelf di secondo ordine
                coeffs = dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, fc, Q, g);
            }
            else if (band == MAX_NUM_BANDS - 1) {
                //high shelf di primo ordine
                coeffs = dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, fc, Q, g);
            }
            else {
                //tutti gli altri peaking
                coeffs = dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, fc, Q, g);
            }

            for (int ch = 0; ch < MAX_NUM_CH; ++ch)
                filters[band].getUnchecked(ch)->coefficients = coeffs;
        }
    }

    double sampleRate = 44100.0;

    std::array<float, MAX_NUM_BANDS> centerFrequencies{};

    std::array<float, MAX_NUM_BANDS> gains{};

    float Q = MathConstants<float>::sqrt2;

    //[banda][canale]
    std::array<OwnedArray<dsp::IIR::Filter<float>>, MAX_NUM_BANDS> filters;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterBank)
};

#define MAX_NUM_CH_ANALYSIS 1

class AnalysisFilterBank {
public:

    AnalysisFilterBank() {
        for (int band = 0; band < MAX_NUM_BANDS; ++band) {
            for (int ch = 0; ch < MAX_NUM_CH_ANALYSIS; ++ch) {
                filters[band].add(new juce::dsp::IIR::Filter<float>());
            }
        }
    }

    ~AnalysisFilterBank() {}

    void prepareToPlay(double sr, const float* fc) {
        sampleRate = sr;

        for (int i = 0; i < MAX_NUM_BANDS; ++i)
            centerFrequencies[i] = fc[i];

        reset();
        updateCoefficients();
    }

    void analyze(const juce::AudioBuffer<float>& buffer, std::array<float, MAX_NUM_BANDS>& analysis) {
        const int numSamples = buffer.getNumSamples();

        for (int band = 0; band < MAX_NUM_BANDS; ++band) {
            //copia buffer
            tempBuffer.makeCopyOf(buffer, true);
            //usiamo solo il primo canale
            tempBuffer.setSize(1, numSamples, true);

            //applico il filtro del caso
            juce::dsp::AudioBlock<float> block(tempBuffer);
            auto chBlock = block.getSingleChannelBlock(0);

            juce::dsp::ProcessContextReplacing<float> context(chBlock);

            filters[band].getUnchecked(0)->process(context);

            //calcolo l'energia di quella banda
            float rms = tempBuffer.getRMSLevel(0, 0, numSamples);

            //la salvo nell'array
            analysis[band] = rms;
        }
    }

    void reset() {
        for (int band = 0; band < MAX_NUM_BANDS; ++band)
            for (int ch = 0; ch < MAX_NUM_CH_ANALYSIS; ++ch)
                filters[band].getUnchecked(ch)->reset();
    }

private:

    void updateCoefficients()
    {
        for (int band = 0; band < MAX_NUM_BANDS; ++band) {
            juce::dsp::IIR::Coefficients<float>::Ptr coeffs;

            const auto fc = centerFrequencies[band];

            if (band == 0) {
                //primo filtro lowpass (per il lowshelf)
                coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, fc, Q);
            }
            else if (band == MAX_NUM_BANDS - 1) {
                //ultimo filtro highpass (per l'high shelf)
                coeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, fc, Q);
            }
            else {
                //i restanti bandpass (per i filtri a campana)
                coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRate, fc, Q);
            }

            for (int ch = 0; ch < MAX_NUM_CH_ANALYSIS; ++ch)
                filters[band].getUnchecked(ch)->coefficients = coeffs;
        }
    }

    double sampleRate = 44100.0;

    std::array<float, MAX_NUM_BANDS> centerFrequencies{};

    float Q = 10;

    juce::AudioBuffer<float> tempBuffer;

    //[banda][canale]
    std::array<juce::OwnedArray<juce::dsp::IIR::Filter<float>>, MAX_NUM_BANDS> filters;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnalysisFilterBank)
};