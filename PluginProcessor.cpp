#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
GraphicEQAudioProcessor::GraphicEQAudioProcessor()
    : parameters(*this, nullptr, "GraphicEQParameters", Parameters::createParameterLayout())
{
    Parameters::addListenerToAllParameters(parameters, this);
    totalGainSmoothed.setCurrentAndTargetValue(1.0f); //che sarebbe 0 dB
}

GraphicEQAudioProcessor::~GraphicEQAudioProcessor()
{
}

//==============================================================================
void GraphicEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    gmc.prepareToPlay(); //calcola internamente commandGain sulla base degli userGain default
    auto fc = gmc.getCenterFrequencies(); //momentaneamente, finchè gmc conterrà la matrice di
                                          //di interazione e le frequenze centrali come look up 
                                          //table (hard coded), sarà solo qui nella PrepareToPlay
                                          //che setteremo le frequenze ai filtri
    filters.prepareToPlay(sampleRate, fc); //crea i 10x2 filtri a campana

    totalGainSmoothed.reset(sampleRate, 0.2f); //Smoothing in 200 ms

    //preparo i buffer ausiliari di learn
    learnLengthSamples = (int)(sampleRate * 5.0); //buffer di 5 secondi

    learnTargetBuffer.setSize(1, learnLengthSamples); //entrambi stereo, se poi arriva mono gestiremo solo mono
    learnReferenceBuffer.setSize(1, learnLengthSamples);

    learnTargetBuffer.clear();
    learnReferenceBuffer.clear();

    learnSamplesTarget = 0; //inizializzati così, gestiti poi dalle funzioni helper del learn
    learnSamplesReference = 0;

    //creo filtri di analisi
    analysisFilters.prepareToPlay(sampleRate, fc);
}

void GraphicEQAudioProcessor::releaseResources()
{

}

void GraphicEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    //separiamo i bus
    auto& targetBuffer = getBusBuffer(buffer, true, 0);
    auto& sidechainBuffer = getBusBuffer(buffer, true, 1);

    //parte che si occupa del learn, qui la sichiama, poi gestisce lei il tutto
    if (learningTarget.load() || learningReference.load()) //controllo che stia avvendendo learning
        handleLearn(targetBuffer, sidechainBuffer);

    const auto numSamples = targetBuffer.getNumSamples();
    const auto numCh = targetBuffer.getNumChannels();

    //processiamo SOLO il target
    filters.processBlock(targetBuffer, numSamples, numCh);

    //G0 è già incluso dentro totalGainSmoothed
    //perché lo setto in updateTotalGainTarget()

    //totalGain
    totalGainSmoothed.applyGain(targetBuffer, numSamples);
}

//==============================================================================
bool GraphicEQAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* GraphicEQAudioProcessor::createEditor()
{
    return new GraphicEQAudioProcessorEditor (*this, parameters, gmc.getCenterFrequencies());
}

//==============================================================================
void GraphicEQAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void GraphicEQAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr) {
        if (xmlState->hasTagName(parameters.state.getType())) {
            parameters.replaceState(ValueTree::fromXml(*xmlState));
        }

    }
}


void GraphicEQAudioProcessor::parameterChanged(const String& paramID, float newValue) {
    //BAND PARAMETERS
    for (int band = 0; band < MAX_NUM_BANDS; ++band)
    {
        if (paramID == Parameters::getBandParamID(band))
        {
            userGain[band] = newValue;

            //aggiorna gain matrix
            gmc.update(userGain);

            //aggiorna filtri
            filters.setGains(gmc.getCommandGain());

            //aggiorna gain totale
            updateTotalGainTarget();

            return;
        }
    }

    if (paramID == Parameters::globalGain) {
        globalGain = newValue;
        updateTotalGainTarget();
    }

    if (paramID == Parameters::useSidechain) {
        useSidechainAsReference.store(newValue);
    }
}

//per triggerare learn e match
void GraphicEQAudioProcessor::triggerLearnTarget() {
    if (!learningTarget.load())
        startLearnTarget();
}

void GraphicEQAudioProcessor::triggerLearnReference() {
    if (!learningReference.load())
        startLearnReference();
}

void GraphicEQAudioProcessor::triggerMatch() {
    juce::MessageManager::callAsync([this] {
        handleMatch();
        });
}

void GraphicEQAudioProcessor::updateTotalGainTarget() { //funzione ausiliaria per il set del target di totalGainSmoothed
    auto G0 = gmc.getGlobalOffset();
    auto totalGainDb = G0 + globalGain;

    auto linearGain = juce::Decibels::decibelsToGain(totalGainDb);
    totalGainSmoothed.setTargetValue(linearGain);
}

//funzione helper per il learn del target (setta i vari booleani)
void GraphicEQAudioProcessor::startLearnTarget() {
    if (learningTarget.load()) //controllo
        return;

    learningTarget.store(true);

    learnSamplesTarget = 0; //si risetta a zero, perchè viene modificato nel corso della fase di learn

    learnTargetBuffer.clear(); //giustamente bisogna ripulirlo da ciò che aveva "imparato" prima

    hasLearnedTarget.store(false);
}
//stessa cosa per la reference
void GraphicEQAudioProcessor::startLearnReference() {
    if (learningReference.load())
        return;

    learningReference.store(true);

    learnSamplesReference = 0;

    learnReferenceBuffer.clear();

    hasLearnedReference.store(false);
}

//funzione ausiliaria vera e propria da richiamare nella processBlock (per tenerla pulita) per gestire tutto il learn
void GraphicEQAudioProcessor::handleLearn(const juce::AudioBuffer<float>& inputBuffer, const juce::AudioBuffer<float>& sidechainBuffer)
{
    const auto numSamples = inputBuffer.getNumSamples();

    //PARTE CHE SI OCCUPA DEL TARGET
    if (learningTarget.load()) {
        int samplesToCopyTarget = std::min(numSamples, learnLengthSamples - learnSamplesTarget);

        if (inputBuffer.getNumChannels() == 1) {
            learnTargetBuffer.addFrom(0, learnSamplesTarget, inputBuffer, 0, 0, samplesToCopyTarget);
        } else if (inputBuffer.getNumChannels() > 1) {
            learnTargetBuffer.addFrom(0, learnSamplesTarget, inputBuffer, 0, 0, samplesToCopyTarget, 0.5f);
            learnTargetBuffer.addFrom(0, learnSamplesTarget, inputBuffer, 1, 0, samplesToCopyTarget, 0.5f);
        }

        learnSamplesTarget += samplesToCopyTarget; //per capire man mano quanto riempirlo

        if (learnSamplesTarget >= learnLengthSamples) {
            learningTarget.store(false);
            hasLearnedTarget.store(true); //servirà come controllo per fare match
        }
    }

    //PARTE CHE SI OCCUPA DELLA REFERENCE
    if (learningReference.load()) {
        //controllo se presente o no sidechain, controllo che input vuole  usare l'utente, e decido cosa mettere
        const bool hasSidechain = sidechainBuffer.getNumChannels() > 0;
        const auto& refBuffer = (useSidechainAsReference.load() && hasSidechain) ? sidechainBuffer : inputBuffer;
        int samplesToCopyReference = std::min(numSamples, learnLengthSamples - learnSamplesReference);

        if (refBuffer.getNumChannels() == 1) {
            learnReferenceBuffer.addFrom(0, learnSamplesReference, refBuffer, 0, 0, samplesToCopyReference);
        } else if (refBuffer.getNumChannels() > 1 ){
            learnReferenceBuffer.addFrom(0, learnSamplesReference, refBuffer, 0, 0, samplesToCopyReference, 0.5f);
            learnReferenceBuffer.addFrom(0, learnSamplesReference, refBuffer, 1, 0, samplesToCopyReference, 0.5f);
        }

        learnSamplesReference += samplesToCopyReference;

        if (learnSamplesReference >= learnLengthSamples) {
            learningReference.store(false);
            hasLearnedReference.store(true);
        }
    }
}

//funzione per gestire l'intero processo di match
void GraphicEQAudioProcessor::handleMatch() {
    DBG("HANDLE MATCH START");
    
    if (!hasLearnedTarget.load() || !hasLearnedReference.load()) {
        DBG("learn non completato");
        return;
    }

    //creo copie dei buffer ausiliari per non alterarli
    juce::AudioBuffer<float> targetBuffer;
    juce::AudioBuffer<float> referenceBuffer;

    targetBuffer.makeCopyOf(learnTargetBuffer);//li copio
    referenceBuffer.makeCopyOf(learnReferenceBuffer);

    //normalizzo
    normalizeBuffer(targetBuffer);
    normalizeBuffer(referenceBuffer);

    DBG("buffer pronti per l'analisi");
    
    //parte di analisi
    std::array<float, MAX_NUM_BANDS> targetAnalysis{}; //creo gli array
    std::array<float, MAX_NUM_BANDS> referenceAnalysis{};
    std::array<float, MAX_NUM_BANDS> eqCurveDb{};

    analysisFilters.reset();
    analysisFilters.analyze(targetBuffer, targetAnalysis); //analizzo
    analysisFilters.reset();
    analysisFilters.analyze(referenceBuffer, referenceAnalysis);

    generateEqCurve(targetAnalysis, referenceAnalysis, eqCurveDb); //creo curva di equalizzazione
    
    //applico la curva di equalizzazione ai vari parametri
    for (int band = 0; band < MAX_NUM_BANDS; ++band) {
        auto paramID = Parameters::getBandParamID(band);

        if (auto* param = parameters.getParameter(paramID)) {
            if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param)) {
                float valueDb = eqCurveDb[band];

                float normalized = floatParam->convertTo0to1(valueDb); //converto per avere il formato giusto

                floatParam->beginChangeGesture();
                floatParam->setValueNotifyingHost(normalized); //così cambiano slider e userGain!
                floatParam->endChangeGesture();
            }
        }
    }
}

void GraphicEQAudioProcessor::normalizeBuffer(juce::AudioBuffer<float>& buffer) {
    const float epsilon = 1e-12f;
    int numSamples = buffer.getNumSamples();

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float rms = buffer.getRMSLevel(ch, 0, numSamples);

        float gain = 1.0f / (rms + epsilon);

        buffer.applyGain(ch, 0, numSamples, gain);
    }
}

void GraphicEQAudioProcessor::generateEqCurve(const std::array<float, MAX_NUM_BANDS>& targetAnalysis, const std::array<float, MAX_NUM_BANDS>& referenceAnalysis, std::array<float, MAX_NUM_BANDS>& eqCurveDb) {
    const float epsilon = 1e-12f;
    const float maxGain = 12.0f;

    for (int i = 0; i < MAX_NUM_BANDS; ++i)
    {
        float target = targetAnalysis[i];
        float reference = referenceAnalysis[i];

        float diff = 20.0f * std::log10(reference / (target + epsilon));

        diff = juce::jlimit(-maxGain, maxGain, diff);

        eqCurveDb[i] = diff;
    }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GraphicEQAudioProcessor();
}
