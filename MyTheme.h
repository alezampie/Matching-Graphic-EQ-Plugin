#pragma once
#include <JuceHeader.h>
#include "BinaryData.h"

#define THUMB_RADIUS 6.0f //macro utili
#define TRACK_WIDTH 4.0f

class MyLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MyLookAndFeel() {
        setColour(juce::Slider::thumbColourId, juce::Colour(0xff4a53b4)); //pallino blu
        setColour(juce::Slider::textBoxTextColourId, juce::Colours::black); //testo nero
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::white); //sfondo bianco

        setColour(juce::TextButton::buttonColourId, juce::Colours::white);
        setColour(juce::TextButton::textColourOffId, juce::Colours::black);
        setColour(juce::TextButton::textColourOnId, juce::Colours::black);

        //font
        auto typeface = juce::Typeface::createSystemTypefaceFor(
            BinaryData::basis33_ttf,
            BinaryData::basis33_ttfSize
        );

        basis33 = juce::Font(typeface);
    }

    //metto font
    juce::Font getLabelFont(juce::Label&) override
    {
        return basis33.withHeight(15.0f);
    }

    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override
    {
        return basis33.withHeight(buttonHeight * 0.45f);
    }

    //edito casella di testo
    void drawLabel(juce::Graphics& g, juce::Label& label) override
    {
        auto bounds = label.getLocalBounds().toFloat();

        g.setColour(label.findColour(juce::Label::backgroundColourId));

        const float cornerRadius = 4.0f; //un leggero radius, qb
        g.fillRoundedRectangle(bounds.reduced(0.5f), cornerRadius);

        //text
        g.setColour(label.findColour(juce::Label::textColourId));
        g.setFont(getLabelFont(label));

        g.drawFittedText(label.getText(),
            label.getLocalBounds(),
            juce::Justification::centred,
            1);
    }

    //edito slider (lo faccio molto semplice)
    void drawLinearSlider(juce::Graphics& g,
        int x, int y, int width, int height,
        float sliderPos,
        float minSliderPos,
        float maxSliderPos,
        const juce::Slider::SliderStyle,
        juce::Slider& slider) override
    {
        //la track occupa tutto lo spazio disponibile dello slider
        const float trackTop = y;
        const float trackBottom = y + height; //nessun margine
        const float trackX = x + width * 0.5f;

        //creo la track
        g.setColour(juce::Colours::black);
        g.fillRect(trackX - TRACK_WIDTH * 0.5f,
            trackTop,
            TRACK_WIDTH,
            trackBottom - trackTop);

        //creo il thumb
        g.setColour(juce::Colour(0xff4a53b4)); //blu come la linea di eq
        g.fillEllipse(trackX - THUMB_RADIUS,
            sliderPos - THUMB_RADIUS,
            THUMB_RADIUS * 2.0f,
            THUMB_RADIUS * 2.0f);
    }

    //qua per i pulsandi di learn e match
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
        const juce::Colour&,
        bool isMouseOverButton, bool isButtonDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);

        //per creare effettp inset
        if (isButtonDown)
            bounds = bounds.translated(0.0f, 2.0f);

        auto colour = button.findColour(juce::TextButton::buttonColourId);

        if (isMouseOverButton)
            colour = colour.brighter(0.05f);

        if (isButtonDown)
            colour = colour.darker(0.2f);

        g.setColour(colour);
        g.fillRoundedRectangle(bounds, 4.0f);

        g.setColour(juce::Colours::black.withAlpha(0.9f));
        g.drawRoundedRectangle(bounds, 4.0f, 1.2f);

        //luce e ombra
        if (!isButtonDown) {
            //luce sopra
            g.setColour(juce::Colours::white.withAlpha(0.25f));
            g.drawLine(bounds.getX() + 1,
                bounds.getY() + 1,
                bounds.getRight() - 1,
                bounds.getY() + 1);

            //ombra sotto
            g.setColour(juce::Colours::black.withAlpha(0.25f));
            g.drawLine(bounds.getX() + 1,
                bounds.getBottom() - 1,
                bounds.getRight() - 1,
                bounds.getBottom() - 1);
        } else {
            //invertito quando premuto (effetto incassato)

            //ombra sopra
            g.setColour(juce::Colours::black.withAlpha(0.25f));
            g.drawLine(bounds.getX() + 1,
                bounds.getY() + 1,
                bounds.getRight() - 1,
                bounds.getY() + 1);

            //luce sotto
            g.setColour(juce::Colours::white.withAlpha(0.15f));
            g.drawLine(bounds.getX() + 1,
                bounds.getBottom() - 1,
                bounds.getRight() - 1,
                bounds.getBottom() - 1);
        }
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& button, bool isMouseOverButton, bool isButtonDown) override {
        g.setFont(getTextButtonFont(button, button.getHeight()).withExtraKerningFactor(0.05f));
        g.setColour(button.findColour(juce::TextButton::textColourOffId));

        g.drawFittedText(button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, 1);
    }

    //per toggle sidechain
    void drawToggleButton(juce::Graphics& g,
        juce::ToggleButton& button,
        bool isMouseOverButton,
        bool isButtonDown) override
    {
        auto bounds = button.getLocalBounds().toFloat();

        const float radius = bounds.getHeight() * 0.5f;

        //sfondo
        juce::Colour trackColour;

        if (button.getToggleState())
            trackColour = juce::Colour(0xff4a53b4); //blu
        else
            trackColour = juce::Colours::darkgrey;

        if (!button.isEnabled())
            trackColour = trackColour.withAlpha(0.3f);

        g.setColour(trackColour);
        g.fillRoundedRectangle(bounds, radius);

        //parte thumb
        float thumbSize = bounds.getHeight() - 6.0f;
        float thumbY = bounds.getY() + (bounds.getHeight() - thumbSize) * 0.5f;

        float thumbX = button.getToggleState()
            ? bounds.getRight() - thumbSize - 3.0f
            : bounds.getX() + 3.0f;

        juce::Colour thumbColour = juce::Colours::white;

        if (isButtonDown)
            thumbColour = thumbColour.darker(0.1f);

        g.setColour(thumbColour);
        g.fillEllipse(thumbX, thumbY, thumbSize, thumbSize);

        //bordo leggero
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.drawRoundedRectangle(bounds, radius, 1.0f);
    }

private:
    juce::Font basis33;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MyLookAndFeel)
};
