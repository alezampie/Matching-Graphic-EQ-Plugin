#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
GraphicEQAudioProcessorEditor::GraphicEQAudioProcessorEditor (GraphicEQAudioProcessor& p, AudioProcessorValueTreeState& vts, const float* centerFreqs)
    : AudioProcessorEditor (&p), audioProcessor (p), valueTreeState(vts), centerFrequencies(centerFreqs)
{
    //setto il font
    auto typeface = juce::Typeface::createSystemTypefaceFor(
        BinaryData::basis33_ttf,
        BinaryData::basis33_ttfSize
    );

    basis33Font = juce::Font(typeface).withHeight(17.0f);

    //layout statico = dimensioni fisse per gli slider
    const int sliderWidth = 40;
    const int sliderHeight = 200;
    const int sliderSpacing = 10;
    const int startX = 20;
    const int startY = 50;

    //ciclo sull'array di attachment
    for (int i = 0; i < MAX_NUM_BANDS; ++i) {
        int x = startX + i * (sliderWidth + sliderSpacing); //ad ogni for lo mette nella posizione giusta
        setupSlider(sliderBand[i], x, startY, sliderWidth, sliderHeight);

        //metto listener
        sliderBand[i].addListener(this);

        //ricavo il paramID usando la funzione helper
        String paramID = Parameters::getBandParamID(i);

        //creo l'attachment
        attachmentBand[i].reset(new SliderAttachment(valueTreeState, paramID, sliderBand[i]));
    }

    //global gain a destra
    int globalX = startX + 10 * (sliderWidth + sliderSpacing) + 30;
    setupSlider(sliderGlobalGain, globalX, startY, sliderWidth, sliderHeight);
    sliderGlobalGain.addListener(this);
    attachmentGlobalGain.reset(new SliderAttachment(valueTreeState, Parameters::globalGain, sliderGlobalGain));

    //per i pulsanti di learn e match
    addAndMakeVisible(learnTargetButton);
    addAndMakeVisible(learnReferenceButton);
    addAndMakeVisible(matchButton);
    //gestione dell'evento
    learnTargetButton.onClick = [this]() { audioProcessor.triggerLearnTarget(); };
    learnReferenceButton.onClick = [this]() { audioProcessor.triggerLearnReference(); };
    matchButton.onClick = [this]() { audioProcessor.triggerMatch(); };

    //per sidechain
    addAndMakeVisible(sidechainToggle);
    sidechainToggle.setSize(45, 16);
    sidechainAttachment.reset(new ButtonAttachment(valueTreeState, Parameters::useSidechain, sidechainToggle));
    addAndMakeVisible(sidechainLabel);
    sidechainLabel.setText("s/c ref", juce::dontSendNotification);
    sidechainLabel.setJustificationType(juce::Justification::centred);

    //look and feel personalizzato
    setLookAndFeel(&myTheme);

    //avvio il timer
    startTimerHz(20);

    setSize(600, 300); //dimensione fissa finestra
}

GraphicEQAudioProcessorEditor::~GraphicEQAudioProcessorEditor()
{
    this->setLookAndFeel(nullptr);
}

void GraphicEQAudioProcessorEditor::paint(juce::Graphics& g)
{
    //setto il font
    g.setFont(basis33Font);

    //sfondo generale
    g.fillAll(juce::Colour(0xffe2d6c8)); // bianco sporco molto plastica anni 70

    //questa funzione ausiliaria fa gran parte della grafica (linee varie)
    drawEQCurve(g);

    //esplicito le frequenze gestite da ogni slider --> dinamico perchè prendo da fc in gainMatrixCalculator e non sono hardcoded qui
    g.setColour(juce::Colours::black);
    const int textHeight = 30;
    const int offsetY = 2;

    for (int i = 0; i < sliderBand.size(); ++i) {
        int x = sliderBand[i].getX();
        int y = sliderBand[i].getBottom() + offsetY;
        String freqText = String(centerFrequencies[i], 1);  //1 decimale
        g.drawFittedText(freqText, x, y, sliderBand[i].getWidth(), textHeight, juce::Justification::centred, 1);
    }

    //global gain
    int gainTextY = sliderGlobalGain.getBottom() + offsetY;
    g.drawFittedText("Global Gain", sliderGlobalGain.getX(), gainTextY, sliderGlobalGain.getWidth(), textHeight, juce::Justification::centred, 2);
}

void GraphicEQAudioProcessorEditor::resized()
{
    const int sliderWidth = 40;
    const int sliderHeight = 200; //queste sono le dimensioni standard degli slider
    const int sliderSpacing = 10;
    const int startX = 20;

    //il global gain però lo faccio più corto ora
    const int globalSliderWidth = 40;
    const int globalSliderHeight = 150; //50 px più corto
    int globalX = startX + sliderBand.size() * (sliderWidth + sliderSpacing) + 20;

    //calcolo la linea a 0 dB degli altri slider
    auto& firstSlider = sliderBand[0];
    int trackTop = firstSlider.getY();
    int trackBottom = firstSlider.getBottom() - firstSlider.getTextBoxHeight();
    float centerY = trackTop + (trackBottom - trackTop) * 0.5f; //la linea a 0 dB è questa

    //calcolo posizione thumb del global gain ridotto -> deve rimanere centrato con gli altri slider anche se è più corto
    float zeroRatio = (0.0f - sliderGlobalGain.getMinimum()) /
        (sliderGlobalGain.getMaximum() - sliderGlobalGain.getMinimum());
    float trackHeightGlobal = globalSliderHeight - sliderGlobalGain.getTextBoxHeight();
    float thumbCenterOffset = zeroRatio * trackHeightGlobal;

    //posiziono il global gain così lo 0 coincide con centerY
    int globalY = (int)(centerY - thumbCenterOffset);
    sliderGlobalGain.setBounds(globalX, globalY, globalSliderWidth, globalSliderHeight);

    //textbox sotto solita
    sliderGlobalGain.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);

    //qui si gestiscono i pulsanti
    const int buttonWidth = 120;
    const int buttonHeight = 30;
    const int marginTop = 15;

    //stessa dimensione textbox slider
    const int textBoxWidth = 50;

    //primo slider già presente, prendo solo l'ultimo slider
    auto& lastSlider = sliderGlobalGain;

    //calcolo lato sinistro e destro "reale" della textBox
    int firstTextBoxLeft = firstSlider.getX() + (firstSlider.getWidth() / 2) - (textBoxWidth / 2);
    int lastTextBoxRight = lastSlider.getX() + (lastSlider.getWidth() / 2) + (textBoxWidth / 2);

    //applico offset per allineamento perfetto
    const int offsetLeft = 5;
    const int offsetRight = -5;

    learnTargetButton.setBounds(firstTextBoxLeft + offsetLeft,
        marginTop,
        buttonWidth,
        buttonHeight);

    learnReferenceButton.setBounds(lastTextBoxRight + offsetRight - buttonWidth,
        marginTop,
        buttonWidth,
        buttonHeight);

    matchButton.setBounds(getWidth() / 2 - buttonWidth / 2,
        marginTop,
        buttonWidth,
        buttonHeight);

    //per toggle sidechain
    int marginRight = 17;
    int toggleWidth = 45;
    int toggleHeight = 16;

    int x = getWidth() - toggleWidth - marginRight;
    int y = getHeight() - 30;

    sidechainToggle.setBounds(x, y, toggleWidth, toggleHeight);
    sidechainLabel.setBounds(x, y + toggleHeight, toggleWidth, 15);
}

void GraphicEQAudioProcessorEditor::sliderValueChanged(juce::Slider*)
{
    repaint(); //ad ogni cambiamento negli slider... faccio repaint per aggiornare
}


void GraphicEQAudioProcessorEditor::setupSlider(juce::Slider& slider, int x, int y, int w, int h)
{
    slider.setSliderStyle(juce::Slider::LinearVertical);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    addAndMakeVisible(slider);
    slider.setBounds(x, y, w, h);
}

void GraphicEQAudioProcessorEditor::drawEQCurve(juce::Graphics& g)
{
    //controllo: se gli slider non sono ancora layoutati, non disegnare nulla
    if (sliderBand[0].getWidth() == 0)
        return;

    juce::Path path; //la linea di eq la faccio usando il path di JUCE

    const int numPoints = (int)sliderBand.size(); //ci saranno sicuramente i punti dei thumb
    std::vector<juce::Point<float>> points;

    //PUNTI X,Y
    //primo punto a sinistra = stesso Y del primo thumb
    float firstX = 0.0f; //il primo punto fisso sta al bordo sx
    float firstY = sliderBand[0].getY() + sliderBand[0].getPositionOfValue(sliderBand[0].getValue());
    points.push_back({ firstX, firstY });

    //i punti centrali corrispondono ai thumb, li ciclo
    for (int i = 0; i < numPoints; ++i) {
        auto& slider = sliderBand[i];
        float x = slider.getX() + slider.getWidth() * 0.5f;
        float y = slider.getY() + slider.getPositionOfValue(slider.getValue());
        points.push_back({ x, y });
    }

    //l' ultimo punto a destra = stesso Y dell'ultimo thumb
    float lastX = (float)getWidth(); //l'ultimo punto fisso sta al bordo dx
    float lastY = sliderBand[numPoints - 1].getY() + sliderBand[numPoints - 1].getPositionOfValue(sliderBand[numPoints - 1].getValue());
    points.push_back({ lastX, lastY });

    //creo i path, ma lo farò smussando gli spigoli
    path.startNewSubPath(points[0]);
    const float cornerRatio = 0.3f; //30% della distanza orizzontale per smussare l'angolo

    for (size_t i = 0; i < points.size() - 1; ++i) {
        juce::Point<float> p0 = points[i];
        juce::Point<float> p1 = points[i + 1];

        //smusso solo per punti centrali (non estremi)
        if (i > 0 && i < points.size() - 2) {
            float dx = (p1.x - p0.x) * cornerRatio;

            juce::Point<float> c1 = { p0.x + dx, p0.y };
            juce::Point<float> c2 = { p1.x - dx, p1.y };

            path.cubicTo(c1, c2, p1);
        } else {
            path.lineTo(p1);
        }
    }

    //creo il path e faccio in modo di riempire fino in basso --> lo faccio puramente per estetica
    juce::Path fillPath = path;
    fillPath.lineTo(lastX, (float)getHeight());   //giù fino al bordo inferiore
    fillPath.lineTo(firstX, (float)getHeight());  //verso il primo punto
    fillPath.closeSubPath();

    //ora riempio di fatto sotto la curva
    g.setColour(juce::Colour(0xffd3784b)); //arancio
    g.fillPath(fillPath);

    //creo un grid orizzontale, che imita un po' i pannelli di EQ analogici
    g.setColour(juce::Colour(0xff4a53b4)); //blu
    g.setOpacity(0.5f);

    const float lineSpacing = 26.0f; //equispazisti di 26 dB
    const int trackTop = sliderBand[0].getY();
    const int trackBottom = sliderBand[0].getBottom() - sliderBand[0].getTextBoxHeight();
    const float centerY = trackTop + (trackBottom - trackTop) * 0.5f;

    //coordinate x = dal centro del primo slider al centro dell' ultimo slider
    const float gridStartX = sliderBand[0].getX() + sliderBand[0].getWidth() * 0.5f;
    const float gridEndX = sliderBand[numPoints - 1].getX() + sliderBand[numPoints - 1].getWidth() * 0.5f;

    g.setColour(juce::Colour(0xff0d5233)); //verde
    g.setOpacity(0.5f);

    g.drawLine(gridStartX, centerY - 2 * lineSpacing, gridEndX, centerY - 2 * lineSpacing, 1.0f);
    g.drawLine(gridStartX, centerY - lineSpacing, gridEndX, centerY - lineSpacing, 1.0f);
    g.drawLine(gridStartX, centerY, gridEndX, centerY, 2.0f); //la centrale più spessa
    g.drawLine(gridStartX, centerY + lineSpacing, gridEndX, centerY + lineSpacing, 1.0f);
    g.drawLine(gridStartX, centerY + 2 * lineSpacing, gridEndX, centerY + 2 * lineSpacing, 1.0f);

    g.setOpacity(1.0f);

    //disegno poi la curva vera di equalizzazione sopra il riempimento (e sopra al grid)
    g.setColour(juce::Colour(0xff4a53b4)); //blu
    g.strokePath(path, juce::PathStrokeType(2.0f));
}

void GraphicEQAudioProcessorEditor::timerCallback()
{
    bool learningTarget = audioProcessor.isLearningTarget();
    bool learningReference = audioProcessor.isLearningReference();

    learnTargetButton.setEnabled(!learningTarget);
    learnReferenceButton.setEnabled(!learningReference);
    sidechainToggle.setEnabled(!learningReference);
    matchButton.setEnabled(!learningReference && !learningTarget);

    learnTargetButton.setColour(
        juce::TextButton::buttonColourId,
        learningTarget ? juce::Colour(0xffd80703) : juce::Colours::white
    );

    learnReferenceButton.setColour(
        juce::TextButton::buttonColourId,
        learningReference ? juce::Colour(0xffd80703) : juce::Colours::white
    );
}