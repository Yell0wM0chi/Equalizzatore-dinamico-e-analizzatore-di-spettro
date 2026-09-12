/*
  ==============================================================================
    PluginEditor.h
  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "VisualizzatoreSpettro.h"

class StileManopoleStudente : public juce::LookAndFeel_V4
{
public:
    StileManopoleStudente()
    {
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff00E5FF));
        setColour(juce::Slider::thumbColourId, juce::Colour(0xffFFD700));
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override
    {
        auto raggio = (float) juce::jmin(width, height) / 2.0f - 10.0f;
        auto centroX = (float) x + (float) width  * 0.5f;
        auto centroY = (float) y + (float) height * 0.5f;
        auto rx = centroX - raggio;
        auto ry = centroY - raggio;
        auto angoloAccensione = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        g.setColour(juce::Colour(0xff222730));
        g.fillEllipse(rx, ry, raggio * 2.0f, raggio * 2.0f);
        
        g.setColour(juce::Colour(0xff12151B));
        g.drawEllipse(rx, ry, raggio * 2.0f, raggio * 2.0f, 2.0f);

        juce::Path arco;
        arco.addCentredArc(centroX, centroY, raggio - 2.0f, raggio - 2.0f, 0.0f, rotaryStartAngle, angoloAccensione, true);
        g.setColour(slider.findColour(juce::Slider::rotarySliderFillColourId));
        g.strokePath(arco, juce::PathStrokeType(3.0f));

        juce::Path linea;
        auto len = raggio * 0.65f;
        linea.startNewSubPath(0.0f, -raggio);
        linea.lineTo(0.0f, -raggio + len);
        linea.applyTransform(juce::AffineTransform::rotation(angoloAccensione).translated(centroX, centroY));
        g.setColour(juce::Colours::white);
        g.strokePath(linea, juce::PathStrokeType(2.5f));
    }
};

class AudioPluginAudioProcessorEditor  : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void configuraManopola(juce::Slider& manopola, const juce::String& idParametro, const juce::String& suffissoUnita);

    AudioPluginAudioProcessor& audioProcessor;
    StileManopoleStudente stileCustom;

    VisualizzatoreSpettro visualizzatoreSpettro;

    juce::TextButton pulsanteCaricaFile { "LOAD AUDIO FILE" };
    juce::TextButton pulsantePlayStop   { "RUN / STOP" };
    std::unique_ptr<juce::FileChooser> selettoreFile;

    juce::Slider manopolaGuadagnoBassi,  manopolaFrequenzaBassi;
    juce::Slider manopolaGuadagnoMedi,   manopolaFrequenzaMedi, manopolaFattoreQMedi;
    juce::Slider manopolaGuadagnoAlti,   manopolaFrequenzaAlti;
    juce::Slider manopolaSaturazioneDrive, manopolaSogliaDinamica;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> collegamentiParametri;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};