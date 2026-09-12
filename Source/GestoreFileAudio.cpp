/*
  ==============================================================================
    GestoreFileAudio.cpp
  ==============================================================================
*/

#include "GestoreFileAudio.h"

GestoreFileAudio::GestoreFileAudio()
{
    formatiAudioSupportati.registerBasicFormats();
}

bool GestoreFileAudio::caricaFileAudioDaDisco(const juce::File& fileSelezionato)
{
    std::unique_ptr<juce::AudioFormatReader> lettore(formatiAudioSupportati.createReaderFor(fileSelezionato));

    if (lettore != nullptr)
    {
        staRiproducendo = false;
        sampleRateFile = lettore->sampleRate;
        bufferTracciaAudio.setSize((int) lettore->numChannels, (int) lettore->lengthInSamples);
        lettore->read(&bufferTracciaAudio, 0, (int) lettore->lengthInSamples, 0, true, true);

        indiceLetturaCampione = 0.0;
        staRiproducendo = true;
        return true;
    }
    return false;
}

float GestoreFileAudio::leggiCampioneConInterpolazione(int canale, double sampleRateDAW)
{
    juce::ignoreUnused(sampleRateDAW);

    if (!staRiproducendo || bufferTracciaAudio.getNumSamples() == 0)
        return 0.0f;

    int totaleCampioni = bufferTracciaAudio.getNumSamples();
    int idx1 = (int) indiceLetturaCampione;
    int idx2 = (idx1 + 1) % totaleCampioni;
    float alpha = (float) (indiceLetturaCampione - idx1);

    int ch = canale % bufferTracciaAudio.getNumChannels();
    float y1 = bufferTracciaAudio.getSample(ch, idx1);
    float y2 = bufferTracciaAudio.getSample(ch, idx2);

    return y1 + alpha * (y2 - y1);
}

void GestoreFileAudio::avanzaPosizioneLettura(double sampleRateDAW)
{
    if (!staRiproducendo || bufferTracciaAudio.getNumSamples() == 0)
        return;

    double passoResampling = sampleRateFile / sampleRateDAW;
    indiceLetturaCampione += passoResampling;

    if (indiceLetturaCampione >= bufferTracciaAudio.getNumSamples())
        indiceLetturaCampione = 0.0;
}
