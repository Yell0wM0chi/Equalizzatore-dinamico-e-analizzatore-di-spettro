/*
  ==============================================================================
    GestoreFileAudio.cpp
    ----------------------------------------------------------------------------
    Qui implementiamo la logica vera e propria per la riproduzione dei file sul computer.
  ==============================================================================
*/

#include "GestoreFileAudio.h"

GestoreFileAudio::GestoreFileAudio()
{
    // Registra i formati audio di base (WAV, AIFF, MP3, ecc.)
    formatiAudioSupportati.registerBasicFormats();
}

// Carica il file selezionato dall'utente nella memoria RAM del computer
bool GestoreFileAudio::caricaFileAudioDaDisco(const juce::File& fileSelezionato)
{
    // Tenta di creare un lettore compatibile con il tipo di file audio
    std::unique_ptr<juce::AudioFormatReader> lettore(formatiAudioSupportati.createReaderFor(fileSelezionato));

    if (lettore != nullptr) // Se il file è stato aperto correttamente
    {
        staRiproducendo = false; // Mette in pausa per un attimo
        sampleRateFile = lettore->sampleRate; // Memorizza la frequenza originale del brano

        // Ridimensiona il buffer di memoria in base ai canali e alla lunghezza del brano
        bufferTracciaAudio.setSize((int) lettore->numChannels, (int) lettore->lengthInSamples);
        
        // Copia i dati sonori dal file alla memoria RAM
        lettore->read(&bufferTracciaAudio, 0, (int) lettore->lengthInSamples, 0, true, true);

        indiceLetturaCampione = 0.0; // Ricomincia dall'inizio della canzone
        staRiproducendo = true;      // Fa partire subito la riproduzione
        return true;
    }
    return false; // Ritorna falso se il file non era valido o supportato
}

// Ritorna il valore del campione corrente. Se la scheda audio va a una velocità diversa,
// calcola il punto preciso tramite interpolazione lineare (media tra due campioni vicini).
float GestoreFileAudio::leggiCampioneConInterpolazione(int canale, double sampleRateDAW)
{
    juce::ignoreUnused(sampleRateDAW);

    // Se siamo in stop o non c'è audio, restituisce silenzio (0)
    if (!staRiproducendo || bufferTracciaAudio.getNumSamples() == 0)
        return 0.0f;

    int totaleCampioni = bufferTracciaAudio.getNumSamples();
    int idx1 = (int) indiceLetturaCampione; // Primo punto vicino
    int idx2 = (idx1 + 1) % totaleCampioni; // Secondo punto vicino
    float alpha = (float) (indiceLetturaCampione - idx1); // Distanza decimale tra i due punti

    int ch = canale % bufferTracciaAudio.getNumChannels();
    float y1 = bufferTracciaAudio.getSample(ch, idx1);
    float y2 = bufferTracciaAudio.getSample(ch, idx2);

    // Formula dell'interpolazione lineare
    return y1 + alpha * (y2 - y1);
}

// Fa avanzare la puntina di lettura nel tempo
void GestoreFileAudio::avanzaPosizioneLettura(double sampleRateDAW)
{
    if (!staRiproducendo || bufferTracciaAudio.getNumSamples() == 0)
        return;

    // Rapporto di velocità tra il file audio e la scheda audio del sistema
    double passoResampling = sampleRateFile / sampleRateDAW;
    indiceLetturaCampione += passoResampling;

    // Se la canzone finisce, riparte dall'inizio (loop)
    if (indiceLetturaCampione >= bufferTracciaAudio.getNumSamples())
        indiceLetturaCampione = 0.0;
}