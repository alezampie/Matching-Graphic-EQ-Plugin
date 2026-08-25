# Development of an Audio Processor for Automatic Equalization Matching

Final Project for the Bachelor's Degree in Sound and Music Computing (Informatica Musicale)  
Università degli Studi di Milano  
Author: Alessio Zampierolo  

**Read the full Thesis (Italian):** [Tesi_Alessio_Zampierolo.pdf](Tesi_Alessio_Zampierolo.pdf)

## Demonstration


https://github.com/user-attachments/assets/e430bf9e-33eb-4cea-90c6-81b1b9c8e7a9



## Abstract

This project presents the design and implementation in C++ of a real-time audio processor, developed using the JUCE framework, conceived to optimize and automate timbral balancing in music production. The project stems from the need to reduce production times during the pre-mix and demo drafting phases, providing the user with a solid and objective sonic starting point.

## The Problem: Interaction Between Fixed-Frequency Bands

The classic architecture of cascaded filters graphic equalizers suffers from an intrinsic mathematical limit: the interaction between adjacent bands (crosstalk or ripple). Because of this phenomenon, the real frequency response of the system does not correspond to the algebraic sum of the gains visually set by the user.

## The Solution: Matrix Correction

To solve this problem, the Digital Signal Processing (DSP) engine of the plugin integrates a mathematical solution based on interaction matrices, exploiting the self-similarity of second-order filters (based on the studies of Abel, Berners, and Välimäki). This implementation calculates the real command gains starting from those set by the user (user gains), guaranteeing an exact correspondence between the visual equalization curve and the one actually applied to the signal.

## The Extension: Cross-Adaptive Automation (Matching EQ)

Once the inaccuracy of the graphic equalizer was resolved, the mathematical engine was extended by introducing a cross-adaptive automation system. 

The plug-in analyzes the frequency profile of an input signal (target) and a reference signal. This measurement occurs over a time span of 5 seconds to obtain a stable and reliable estimate. Subsequently, after converting the signals to mono and aligning their volumes (RMS normalization), the system compares the energies on the individual ISO bands using a dedicated measurement filter bank, and automatically applies the necessary equalization curve. 

The result is an instrument capable of autonomously modifying the parameters of the graphic equalizer in order to make the timbral profile of the target converge towards that of the reference.

## Technologies and Tools

* **C++ and JUCE framework:** Used for the audio programming, DSP module architecture, and graphical user interface.
* **Eigen library:** Used for linear algebra and matrix calculations required by the interaction matrix.
* **Filter Topology:** Peaking, Low-Shelf, and High-Shelf IIR filters for the equalization path, Band-Pass, Low-Pass, and High-Pass filters for the analysis path.
* **MATLAB (Prototypes):** Used for further developments, including Mid/Side spatial processing and the introduction of arbitrary frequency scales.
