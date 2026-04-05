#pragma once
#include <JuceHeader.h> 
#include "PluginParameters.h"

#include <Eigen/Dense>


class GainMatrixCalculator {
public:
    GainMatrixCalculator() {
        //creo le look up table
        centerFrequencies <<
            31.5f, 63.0f, 125.0f, 250.0f, 500.0f,
            1000.0f, 2000.0f, 4000.0f, 8000.0f, 16000.0f;

        interactionMatrix <<
            0.80f, 0.23f, 0.02f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f,
            0.19f, 1.00f, 0.21f, 0.04f, 0.01f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f,
            0.04f, 0.21f, 1.00f, 0.20f, 0.04f, 0.01f, 0.00f, 0.00f, 0.00f, 0.00f,
            0.01f, 0.04f, 0.20f, 1.00f, 0.20f, 0.04f, 0.01f, 0.00f, 0.00f, 0.00f,
            0.00f, 0.01f, 0.04f, 0.20f, 1.00f, 0.20f, 0.04f, 0.01f, 0.00f, 0.00f,
            0.00f, 0.00f, 0.01f, 0.04f, 0.20f, 1.00f, 0.20f, 0.04f, 0.01f, 0.00f,
            0.00f, 0.00f, 0.00f, 0.01f, 0.04f, 0.20f, 1.00f, 0.20f, 0.03f, 0.00f,
            0.00f, 0.00f, 0.00f, 0.00f, 0.01f, 0.04f, 0.21f, 1.00f, 0.18f, 0.01f,
            0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.01f, 0.06f, 0.25f, 1.00f, 0.10f,
            0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.10f, 0.14f, 0.94f;

            commandGain.setZero();//di default a zero
            G0 = 0.0f; 


    }
    ~GainMatrixCalculator() {

    }

    void update(const float* userGain)
    {
        //faccio questo perchli passo un array ma qui lavoro in eigen
        Eigen::Map<const Eigen::Array<float, MAX_NUM_BANDS, 1>> ug(userGain);

        //calcolo G0
        G0 = ug.mean();

        //in una nuova variabile metto gli user gain con l'offset dato da G0
        Eigen::Array<float, MAX_NUM_BANDS, 1> ugOffset = ug - G0;

        //calcolo i command gain sulla base della matrice di interazione
        commandGain = (interactionMatrix * ugOffset.matrix()).array();
    }

    const float* getCommandGain() const
    {
        return commandGain.data();
    }

    const float* getCenterFrequencies() const
    {
        return centerFrequencies.data();
    }

    float getGlobalOffset() const
    {
        return G0;
    }

    void prepareToPlay() {} //per estensioni future

private:
    Eigen::Matrix<float, MAX_NUM_BANDS, MAX_NUM_BANDS> interactionMatrix;
    Eigen::Array<float, MAX_NUM_BANDS, 1> centerFrequencies;
    Eigen::Array<float, MAX_NUM_BANDS, 1> commandGain;

    float G0 = 0.0f;
};