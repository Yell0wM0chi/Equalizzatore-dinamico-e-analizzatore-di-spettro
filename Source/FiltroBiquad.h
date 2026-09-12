/*
  ==============================================================================
    FiltroBiquad.h
    Progetto DSP - Modulo Filtro IIR di Secondo Ordine (Biquad)
  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class FiltroBiquad
{
public:
    FiltroBiquad() = default;

    void azzeraMemoriaFiltro()
    {
        statoRitardo1 = 0.0f;
        statoRitardo2 = 0.0f;
    }

    float elaboraCampione(float campioneIngresso)
    {
        float valoreIntermedio = campioneIngresso - coeffA1 * statoRitardo1 - coeffA2 * statoRitardo2;
        float campioneUscita = coeffB0 * valoreIntermedio + coeffB1 * statoRitardo1 + coeffB2 * statoRitardo2;

        statoRitardo2 = statoRitardo1;
        statoRitardo1 = valoreIntermedio;

        return campioneUscita;
    }

    float calcolaRispostaInFrequenza_dB(float frequenzaTest_Hz, float sampleRateSistema) const
    {
        float omega = 2.0f * juce::MathConstants<float>::pi * frequenzaTest_Hz / sampleRateSistema;
        float cosw = std::cos(omega);
        float sinw = std::sin(omega);
        float cos2w = std::cos(2.0f * omega);
        float sin2w = std::sin(2.0f * omega);

        float reNum = coeffB0 + coeffB1 * cosw + coeffB2 * cos2w;
        float imNum = -coeffB1 * sinw - coeffB2 * sin2w;
        float reDen = 1.0f + coeffA1 * cosw + coeffA2 * cos2w;
        float imDen = -coeffA1 * sinw - coeffA2 * sin2w;

        float modNum = std::sqrt(reNum * reNum + imNum * imNum);
        float modDen = std::sqrt(reDen * reDen + imDen * imDen);

        float guadagnoLineare = modNum / juce::jmax(0.00001f, modDen);
        return juce::Decibels::gainToDecibels(guadagnoLineare);
    }

    void impostaLowShelf(float sampleRate, float frequenzaTaglio_Hz, float guadagno_dB)
    {
        float A = std::pow(10.0f, guadagno_dB / 40.0f);
        float omega0 = 2.0f * juce::MathConstants<float>::pi * frequenzaTaglio_Hz / sampleRate;
        float cosw0 = std::cos(omega0);
        float sinw0 = std::sin(omega0);
        float alfa = sinw0 / 2.0f * std::sqrt(2.0f);

        float a0 = (A + 1.0f) + (A - 1.0f) * cosw0 + 2.0f * std::sqrt(A) * alfa;

        coeffB0 = (A * ((A + 1.0f) - (A - 1.0f) * cosw0 + 2.0f * std::sqrt(A) * alfa)) / a0;
        coeffB1 = (2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosw0)) / a0;
        coeffB2 = (A * ((A + 1.0f) - (A - 1.0f) * cosw0 - 2.0f * std::sqrt(A) * alfa)) / a0;
        coeffA1 = (-2.0f * ((A - 1.0f) + (A + 1.0f) * cosw0)) / a0;
        coeffA2 = ((A + 1.0f) + (A - 1.0f) * cosw0 - 2.0f * std::sqrt(A) * alfa) / a0;
    }

    void impostaPeak(float sampleRate, float frequenzaCentrale_Hz, float guadagno_dB, float fattoreQ)
    {
        float A = std::pow(10.0f, guadagno_dB / 40.0f);
        float omega0 = 2.0f * juce::MathConstants<float>::pi * frequenzaCentrale_Hz / sampleRate;
        float cosw0 = std::cos(omega0);
        float alfa = std::sin(omega0) / (2.0f * juce::jmax(0.01f, fattoreQ));

        float a0 = 1.0f + alfa / A;

        coeffB0 = (1.0f + alfa * A) / a0;
        coeffB1 = (-2.0f * cosw0) / a0;
        coeffB2 = (1.0f - alfa * A) / a0;
        coeffA1 = (-2.0f * cosw0) / a0;
        coeffA2 = (1.0f - alfa / A) / a0;
    }

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
    float coeffB0 = 1.0f, coeffB1 = 0.0f, coeffB2 = 0.0f;
    float coeffA1 = 0.0f, coeffA2 = 0.0f;
    float statoRitardo1 = 0.0f, statoRitardo2 = 0.0f;
};