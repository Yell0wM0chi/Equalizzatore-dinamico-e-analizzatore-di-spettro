/*
  ==============================================================================
    VisualizzatoreSpettro.h
    Progetto DSP - Display Grafico Reattivo con Clip Region Rigida
    ----------------------------------------------------------------------------
    SPIEGAZIONE SEMPLICE:   Dichiara le funzioni per calcolare lo spettro di frequenza (FFT) e tracciare
    i quattro grafici visivi: oscilloscopio, spettro FFT, curva EQ e saturazione.
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

    // Riceve i campioni dall'engine audio e calcola la FFT
    void riceviCampioniAudioPerFFT(const float* bufferIngresso, int dimensioneBuffer);

    // Riceve i filtri per calcolare la curva dell'equalizzatore
    void impostaFiltriDiRiferimento(FiltroBiquad low, FiltroBiquad mid, FiltroBiquad high, float sampleRate, float drive);

    void paint(juce::Graphics& g) override;
    void resized() override {}

private:
    void timerCallback() override;

    // Disegna la griglia di sfondo con gli assi numerici
    juce::Rectangle<float> disegnaGrigliaEAssi(juce::Graphics& g, juce::Rectangle<float> area, juce::String titolo, juce::String etichettaY);

    // Le 4 funzioni di disegno dei singoli grafici
    void disegnaOscilloscopioLive(juce::Graphics& g, juce::Rectangle<float> area);
    void disegnaSpettroFFTLive(juce::Graphics& g, juce::Rectangle<float> area);
    void disegnaCurvaEQLive(juce::Graphics& g, juce::Rectangle<float> area);
    void disegnaSaturazioneLive(juce::Graphics& g, juce::Rectangle<float> area);

    // Dimensione del blocco FFT: 2^10 = 1024 punti
    static constexpr int ordineFFT = 10;
    static constexpr int dimensioneFFT = 1 << ordineFFT;

    juce::dsp::FFT calcolatoreFFT { ordineFFT }; // Algoritmo per la trasformata di Fourier
    juce::dsp::WindowingFunction<float> finestraDiHann { dimensioneFFT, juce::dsp::WindowingFunction<float>::hann }; // Finestra per smussare i dati audio

    float bufferWaveformLive[dimensioneFFT];           // Dati per l'oscilloscopio
    float campioniTempFFT[2 * dimensioneFFT];           // Buffer di lavoro della FFT
    float livelloSpettroIstantaneo[dimensioneFFT / 2]; // Livello delle frequenze in tempo reale
    float tracciaPicchiMassimi[dimensioneFFT / 2];     // Linea dei picchi (Peak Hold)

    FiltroBiquad filtroLowRif, filtroMidRif, filtroHighRif;
    float sampleRateRiferimento = 48000.0f;
    float valoreDriveRiferimento = 1.0f;
    float ampiezzaPiccoIstantaneo = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VisualizzatoreSpettro)
};