/*
  ==============================================================================
    PluginProcessor.h
    Progetto DSP - Engine Audio Principale
    ----------------------------------------------------------------------------
    Questo file è la "scheda madre" dell'audio. Dichiara i filtri, i parametri
    delle manopole e i dati inviati alla finestra visiva del plugin.
  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "FiltroBiquad.h"
#include "GestoreFileAudio.h"

class AudioPluginAudioProcessor  : public juce::AudioProcessor
{
public:
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    // Funzioni trasversali JUCE per l'avvio e l'elaborazione audio
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // Crea l'interfaccia grafica
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    // Informazioni generali sul plugin
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    // Funzioni per salvare e ricaricare lo stato delle manopole quando si salva un progetto
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    GestoreFileAudio gestoreFileAudio; // Riproduttore del file audio
    
    static constexpr int dimensioneFFT = 1024; // Numero di campioni elaborati per la grafica
    float bufferFifoPerGUI[dimensioneFFT];    // Buffer di passaggio dati audio alla GUI
    int indiceBufferFifo = 0;
    bool bloccoAudioProntoPerGUI = false;     // Segnale per avvisare la grafica che ci sono nuovi dati

    // Copia dei filtri inviata alla GUI per disegnare la curva azzurra dell'EQ
    FiltroBiquad filtroLowRef, filtroMidRef, filtroHighRef;

    // Struttura che contiene e gestisce tutte le manopole (interfaccia-audio)
    juce::AudioProcessorValueTreeState alberoParametriPlugin;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout creaParametriIniziali();

    // Filtri Biquad separati per canale Sinistro (L) e Destro (R)
    FiltroBiquad filtroBassiL, filtroBassiR;
    FiltroBiquad filtroMediL,  filtroMediR;
    FiltroBiquad filtroAltiL,  filtroAltiR;

    float inviluppoLivelloMedi = 0.0f; // Livello di volume attuale per la compressione dei medi
    double sampleRateDAW = 44100.0;   // Frequenza di campionamento del programma principale

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
