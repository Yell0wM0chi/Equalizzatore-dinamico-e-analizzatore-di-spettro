/*
  ==============================================================================
    VisualizzatoreSpettro.h
    Progetto DSP - Display Grafico Reattivo con Clip Region Rigida
  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "FiltroBiquad.h"

class VisualizzatoreSpettro : public juce::Component, private juce::Timer
{
public:
    VisualizzatoreSpettro();
    ~VisualizzatoreSpettro() override;

    void riceviCampioniAudioPerFFT(const float* bufferIngresso, int dimensioneBuffer);
    void impostaFiltriDiRiferimento(FiltroBiquad low, FiltroBiquad mid, FiltroBiquad high, float sampleRate, float drive);

    void paint(juce::Graphics& g) override;
    void resized() override {}

private:
    void timerCallback() override;

    juce::Rectangle<float> disegnaGrigliaEAssi(juce::Graphics& g, juce::Rectangle<float> area, juce::String titolo, juce::String etichettaY);

    void disegnaOscilloscopioLive(juce::Graphics& g, juce::Rectangle<float> area);
    void disegnaSpettroFFTLive(juce::Graphics& g, juce::Rectangle<float> area);
    void disegnaCurvaEQLive(juce::Graphics& g, juce::Rectangle<float> area);
    void disegnaSaturazioneLive(juce::Graphics& g, juce::Rectangle<float> area);

    static constexpr int ordineFFT = 10;
    static constexpr int dimensioneFFT = 1 << ordineFFT; // 1024 campioni

    juce::dsp::FFT calcolatoreFFT { ordineFFT };
    juce::dsp::WindowingFunction<float> finestraDiHann { dimensioneFFT, juce::dsp::WindowingFunction<float>::hann };

    float bufferWaveformLive[dimensioneFFT];
    float campioniTempFFT[2 * dimensioneFFT];
    float livelloSpettroIstantaneo[dimensioneFFT / 2];
    float tracciaPicchiMassimi[dimensioneFFT / 2];

    FiltroBiquad filtroLowRif, filtroMidRif, filtroHighRif;
    float sampleRateRiferimento = 48000.0f;
    float valoreDriveRiferimento = 1.0f;
    float ampiezzaPiccoIstantaneo = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VisualizzatoreSpettro)
};