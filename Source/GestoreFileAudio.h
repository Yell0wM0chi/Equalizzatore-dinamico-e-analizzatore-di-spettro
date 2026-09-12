/*
  ==============================================================================
    GestoreFileAudio.h
    Progetto DSP - Lettura File Audio e Resampling Lineare
  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class GestoreFileAudio
{
public:
    GestoreFileAudio();

    bool caricaFileAudioDaDisco(const juce::File& fileSelezionato);
    float leggiCampioneConInterpolazione(int canale, double sampleRateDAW);
    void avanzaPosizioneLettura(double sampleRateDAW);

    void impostaRiproduzioneAttiva(bool attivo) { staRiproducendo = attivo; }
    bool isRiproduzioneAttiva() const { return staRiproducendo; }

private:
    juce::AudioFormatManager formatiAudioSupportati;
    juce::AudioBuffer<float> bufferTracciaAudio;
    double sampleRateFile = 44100.0;
    double indiceLetturaCampione = 0.0;
    bool staRiproducendo = false;
};