/*
  ==============================================================================
    FiltroBiquad.h
    Progetto DSP - Modulo Filtro IIR di Secondo Ordine (Biquad)
    ----------------------------------------------------------------------------
    Questo file definisce il "mattoncino" matematico per filtrare il suono.
    Un filtro Biquad prende un campione audio alla volta e lo modifica moltiplicandolo
    per dei numeri (coefficienti), ricordandosi anche dei campioni audio passati.
  ==============================================================================
*/

#pragma once 
#include <JuceHeader.h> 

class FiltroBiquad
{
public:
    // Costruttore di default: crea il filtro con impostazioni iniziali neutre
    FiltroBiquad() = default;

    // Resetta la "memoria" del filtro.
    // Il filtro ricorda i campioni scorsi; azzerarli serve a evitare scoppiettii o rumori insoliti all'avvio.
    void azzeraMemoriaFiltro()
    {
        statoRitardo1 = 0.0f; // Azzera il ricordo del campione appena passato
        statoRitardo2 = 0.0f; // Azzera il ricordo di due campioni fa
    }

    // Questa è la funzione che elabora un singolo campione di audio (un frammento infinitesimo di suono).
    float elaboraCampione(float campioneIngresso)
    {
        // Formula matematica (Direct Form II): combina l'ingresso attuale con il ricordo dei campioni passati
        float valoreIntermedio = campioneIngresso - coeffA1 * statoRitardo1 - coeffA2 * statoRitardo2;
        
        // Calcola il punto di suono finale applicando i coefficienti 'B'
        float campioneUscita = coeffB0 * valoreIntermedio + coeffB1 * statoRitardo1 + coeffB2 * statoRitardo2;

        // Fai scorrere la memoria indietro nel tempo per prepararti al prossimo campione
        statoRitardo2 = statoRitardo1;      // Quello che era "ieri" diventa "l'altro ieri"
        statoRitardo1 = valoreIntermedio;   // Il valore attuale diventa "ieri"

        return campioneUscita; // Restituisce il frammento di audio modificato
    }

    // Calcola di quanti decibel (dB) il filtro aumenta o riduce una specifica frequenza (Hz).
    // Serve per poter disegnare a schermo la curva azzurra dell'equalizzatore.
    float calcolaRispostaInFrequenza_dB(float frequenzaTest_Hz, float sampleRateSistema) const
    {
        // Converte la frequenza in Hz in radianti (omega)
        float omega = 2.0f * juce::MathConstants<float>::pi * frequenzaTest_Hz / sampleRateSistema;
        float cosw = std::cos(omega);
        float sinw = std::sin(omega);
        float cos2w = std::cos(2.0f * omega);
        float sin2w = std::sin(2.0f * omega);

        // Calcolo dei numeri complessi (parte reale e immaginaria) per numeratore e denominatore
        float reNum = coeffB0 + coeffB1 * cosw + coeffB2 * cos2w;
        float imNum = -coeffB1 * sinw - coeffB2 * sin2w;
        float reDen = 1.0f + coeffA1 * cosw + coeffA2 * cos2w;
        float imDen = -coeffA1 * sinw - coeffA2 * sin2w;

        // Calcola l'ampiezza (modulo)
        float modNum = std::sqrt(reNum * reNum + imNum * imNum);
        float modDen = std::sqrt(reDen * reDen + imDen * imDen);

        // Evita divisioni per zero ed esprime il risultato in Decibel (dB)
        float guadagnoLineare = modNum / juce::jmax(0.00001f, modDen);
        return juce::Decibels::gainToDecibels(guadagnoLineare);
    }

    // Imposta il filtro per lavorare sui BASSI (Low Shelf)
    void impostaLowShelf(float sampleRate, float frequenzaTaglio_Hz, float guadagno_dB)
    {
        float A = std::pow(10.0f, guadagno_dB / 40.0f); // Converte i dB in guadagno lineare
        float omega0 = 2.0f * juce::MathConstants<float>::pi * frequenzaTaglio_Hz / sampleRate;
        float cosw0 = std::cos(omega0);
        float sinw0 = std::sin(omega0);
        float alfa = sinw0 / 2.0f * std::sqrt(2.0f);

        float a0 = (A + 1.0f) + (A - 1.0f) * cosw0 + 2.0f * std::sqrt(A) * alfa;

        // Calcolo dei coefficienti standard dell'Audio EQ Cookbook
        coeffB0 = (A * ((A + 1.0f) - (A - 1.0f) * cosw0 + 2.0f * std::sqrt(A) * alfa)) / a0;
        coeffB1 = (2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosw0)) / a0;
        coeffB2 = (A * ((A + 1.0f) - (A - 1.0f) * cosw0 - 2.0f * std::sqrt(A) * alfa)) / a0;
        coeffA1 = (-2.0f * ((A - 1.0f) + (A + 1.0f) * cosw0)) / a0;
        coeffA2 = ((A + 1.0f) + (A - 1.0f) * cosw0 - 2.0f * std::sqrt(A) * alfa) / a0;
    }

    // Imposta il filtro per lavorare sui MEDI (Filtro a Campana / Peak EQ)
    void impostaPeak(float sampleRate, float frequenzaCentrale_Hz, float guadagno_dB, float fattoreQ)
    {
        float A = std::pow(10.0f, guadagno_dB / 40.0f);
        float omega0 = 2.0f * juce::MathConstants<float>::pi * frequenzaCentrale_Hz / sampleRate;
        float cosw0 = std::cos(omega0);
        // 'fattoreQ' regola quanto la campana d'azione del filtro deve essere stretta o larga
        float alfa = std::sin(omega0) / (2.0f * juce::jmax(0.01f, fattoreQ));

        float a0 = 1.0f + alfa / A;

        coeffB0 = (1.0f + alfa * A) / a0;
        coeffB1 = (-2.0f * cosw0) / a0;
        coeffB2 = (1.0f - alfa * A) / a0;
        coeffA1 = (-2.0f * cosw0) / a0;
        coeffA2 = (1.0f - alfa / A) / a0;
    }

    // Imposta il filtro per lavorare sugli ALTI (High Shelf)
    void impostaHighShelf(float sampleRate, float frequenzaTaglio_Hz, float guadagno_dB)
    {
        float A = std::pow(10.0f, guadagno_dB / 40.0f);
        float omega0 = 2.0f * juce::MathConstants<float>::pi * frequenzaTaglio_Hz / sampleRate;
        float cosw0 = std::cos(omega0);
        float sinw0 = std::sin(omega0);
        float alfa = sinw0 / 2.0f * std::sqrt(2.0f);

        float a0 = (A + 1.0f) - (A - 1.0f) * cosw0 + 2.0f * std::sqrt(A) * alfa;

        coeffB0 = (A * ((A + 1.0f) + (A - 1.0f) * cosw0 + 2.0f * std::sqrt(A) * alfa)) / a0;
        coeffB1 = (-2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosw0)) / a0;
        coeffB2 = (A * ((A + 1.0f) + (A - 1.0f) * cosw0 - 2.0f * std::sqrt(A) * alfa)) / a0;
        coeffA1 = (2.0f * ((A - 1.0f) - (A + 1.0f) * cosw0)) / a0;
        coeffA2 = ((A + 1.0f) - (A - 1.0f) * cosw0 - 2.0f * std::sqrt(A) * alfa) / a0;
    }

private:
    // Coefficienti del filtro (i moltiplicatori dell'equazione)
    float coeffB0 = 1.0f, coeffB1 = 0.0f, coeffB2 = 0.0f;
    float coeffA1 = 0.0f, coeffA2 = 0.0f;

    // Memorie dei campioni audio precedenti
    float statoRitardo1 = 0.0f;
    float statoRitardo2 = 0.0f;
};