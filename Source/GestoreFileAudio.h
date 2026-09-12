/*
  ==============================================================================
    GestoreFileAudio.h
    Progetto DSP - Lettura File Audio e Resampling Lineare
    ----------------------------------------------------------------------------
    Questo file dichiara la classe usata per caricare e leggere i brani audio
    selezionati dal computer, adattando la velocità se la scheda audio lavora
    a una frequenza diversa rispetto a quella del file.
  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class GestoreFileAudio
{
public:
    GestoreFileAudio(); // Costruttore

    // Carica un file MP3/WAV/FLAC dalla memoria del computer
    bool caricaFileAudioDaDisco(const juce::File& fileSelezionato);

    // Ritorna il valore audio di un singolo punto della canzone, stimando il valore se le frequenze differiscono
    float leggiCampioneConInterpolazione(int canale, double sampleRateDAW);

    // Muove in avanti la "puntina" di lettura lungo la canzone
    void avanzaPosizioneLettura(double sampleRateDAW);

    // Permette di mettere in PLAY o in PAUSA
    void impostaRiproduzioneAttiva(bool attivo) { staRiproducendo = attivo; }
    
    // Dice se la musica sta suonando oppure no
    bool isRiproduzioneAttiva() const { return staRiproducendo; }

private:
    juce::AudioFormatManager formatiAudioSupportati; // Strumento JUCE che riconosce i tipi di file
    juce::AudioBuffer<float> bufferTracciaAudio;      // Il contenitore in RAM con tutta la canzone caricata
    double sampleRateFile = 44100.0;                 // Frequenza di campionamento del file (es. 44.1 kHz)
    double indiceLetturaCampione = 0.0;              // La posizione attuale della puntina di lettura
    bool staRiproducendo = false;                    // Interruttore Play/Stop
};