/*
  ==============================================================================
    VisualizzatoreSpettro.cpp
  ==============================================================================
*/

#include "VisualizzatoreSpettro.h"

VisualizzatoreSpettro::VisualizzatoreSpettro()
{
    juce::zeromem(bufferWaveformLive, sizeof(bufferWaveformLive));
    juce::zeromem(livelloSpettroIstantaneo, sizeof(livelloSpettroIstantaneo));
    juce::zeromem(tracciaPicchiMassimi, sizeof(tracciaPicchiMassimi));
    startTimerHz(60);
}

VisualizzatoreSpettro::~VisualizzatoreSpettro() {}

void VisualizzatoreSpettro::riceviCampioniAudioPerFFT(const float* bufferIngresso, int dimensioneBuffer)
{
    if (bufferIngresso == nullptr || dimensioneBuffer <= 0)
        return;

    int campioniDaCopiare = std::min(dimensioneBuffer, dimensioneFFT);

    std::memcpy(bufferWaveformLive, bufferIngresso, sizeof(float) * static_cast<size_t>(campioniDaCopiare));

    // Calcolo del picco istantaneo per il tracciamento sul grafico di saturazione
    float maxAmp = 0.0f;
    for (int i = 0; i < campioniDaCopiare; ++i)
    {
        float absVal = std::abs(bufferIngresso[i]);
        if (absVal > maxAmp)
            maxAmp = absVal;
    }
    ampiezzaPiccoIstantaneo = maxAmp;

    juce::zeromem(campioniTempFFT, sizeof(campioniTempFFT));
    std::memcpy(campioniTempFFT, bufferIngresso, sizeof(float) * static_cast<size_t>(campioniDaCopiare));

    finestraDiHann.multiplyWithWindowingTable(campioniTempFFT, dimensioneFFT);
    calcolatoreFFT.performFrequencyOnlyForwardTransform(campioniTempFFT);

    for (int i = 0; i < dimensioneFFT / 2; ++i)
    {
        float livelloIn_dB = juce::Decibels::gainToDecibels(campioniTempFFT[i]) - juce::Decibels::gainToDecibels(static_cast<float>(dimensioneFFT));
        float valoreNorm = juce::jmap(juce::jlimit(-70.0f, 0.0f, livelloIn_dB), -70.0f, 0.0f, 0.0f, 1.0f);

        livelloSpettroIstantaneo[i] = valoreNorm;

        if (valoreNorm > tracciaPicchiMassimi[i])
            tracciaPicchiMassimi[i] = valoreNorm;
        else
            tracciaPicchiMassimi[i] *= 0.98f;
    }
}

void VisualizzatoreSpettro::impostaFiltriDiRiferimento(FiltroBiquad low, FiltroBiquad mid, FiltroBiquad high, float sampleRate, float drive)
{
    filtroLowRif = low;
    filtroMidRif = mid;
    filtroHighRif = high;
    sampleRateRiferimento = sampleRate;
    valoreDriveRiferimento = drive;
}

void VisualizzatoreSpettro::timerCallback()
{
    repaint();
}

juce::Rectangle<float> VisualizzatoreSpettro::disegnaGrigliaEAssi(juce::Graphics& g, juce::Rectangle<float> area, juce::String titolo, juce::String etichettaY)
{
    g.setColour(juce::Colour(0xff12161F));
    g.fillRoundedRectangle(area, 6.0f);

    g.setColour(juce::Colour(0xff2A3240));
    g.drawRoundedRectangle(area, 6.0f, 1.0f);

    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.setColour(juce::Colours::white);
    g.drawText(titolo, static_cast<int>(area.getX()), static_cast<int>(area.getY() + 4.0f), static_cast<int>(area.getWidth()), 16, juce::Justification::centred);

    float marginX = 32.0f;
    float marginYTop = 22.0f;
    float marginYBot = 16.0f;

    juce::Rectangle<float> areaGrafico(area.getX() + marginX,
                                       area.getY() + marginYTop,
                                       area.getWidth() - marginX - 10.0f,
                                       area.getHeight() - marginYTop - marginYBot);

    g.setFont(juce::FontOptions(9.0f, juce::Font::plain));
    const float valoriY[] = { 1.0f, 0.5f, 0.0f, -0.5f, -1.0f };

    for (float val : valoriY)
    {
        float normY = juce::jmap(val, -1.0f, 1.0f, 1.0f, 0.0f);
        float yPos = areaGrafico.getY() + normY * areaGrafico.getHeight();

        g.setColour(juce::Colour(0xff222B38));
        g.drawHorizontalLine(static_cast<int>(yPos), areaGrafico.getX(), areaGrafico.getRight());

        g.setColour(juce::Colour(0xff7A8A9E));
        juce::String txtY = (etichettaY == "dB") ? juce::String(static_cast<int>(val * 24.0f)) : juce::String(val, 1);
        g.drawText(txtY, static_cast<int>(area.getX() + 2.0f), static_cast<int>(yPos - 6.0f), static_cast<int>(marginX - 4.0f), 12, juce::Justification::right);
    }

    const float valoriX[] = { 0.00f, 0.25f, 0.50f, 0.75f, 1.00f };

    for (float val : valoriX)
    {
        float xPos = areaGrafico.getX() + val * areaGrafico.getWidth();

        g.setColour(juce::Colour(0xff222B38));
        g.drawVerticalLine(static_cast<int>(xPos), areaGrafico.getY(), areaGrafico.getBottom());

        g.setColour(juce::Colour(0xff7A8A9E));
        g.drawText(juce::String(val, 2), static_cast<int>(xPos - 15.0f), static_cast<int>(areaGrafico.getBottom() + 1.0f), 30, 12, juce::Justification::centred);
    }

    return areaGrafico;
}

void VisualizzatoreSpettro::disegnaOscilloscopioLive(juce::Graphics& g, juce::Rectangle<float> area)
{
    auto plot = disegnaGrigliaEAssi(g, area, "1. OSCILLOSCOPIO LIVE (Tempo)", "Ampl");

    // RITAGLIO RIGIDO: Garantisce che la linea non esca mai dai bordi del rettangolo
    juce::Graphics::ScopedSaveState saveState(g);
    g.reduceClipRegion(plot.toNearestInt());

    juce::Path traccia;
    bool primo = true;
    int larghezzaPixel = static_cast<int>(plot.getWidth());

    for (int x = 0; x < larghezzaPixel; ++x)
    {
        float normX = static_cast<float>(x) / static_cast<float>(larghezzaPixel);
        int sampleIdx = static_cast<int>(normX * (dimensioneFFT - 1));
        float val = bufferWaveformLive[sampleIdx];

        float xPos = plot.getX() + static_cast<float>(x);
        float rawY = plot.getY() + juce::jmap(val, -1.0f, 1.0f, 1.0f, 0.0f) * plot.getHeight();
        float yPos = juce::jlimit(plot.getY(), plot.getBottom(), rawY);

        if (primo) { traccia.startNewSubPath(xPos, yPos); primo = false; }
        else { traccia.lineTo(xPos, yPos); }
    }

    g.setColour(juce::Colour(0xff2962FF));
    g.strokePath(traccia, juce::PathStrokeType(1.6f));
}

void VisualizzatoreSpettro::disegnaSpettroFFTLive(juce::Graphics& g, juce::Rectangle<float> area)
{
    auto plot = disegnaGrigliaEAssi(g, area, "2. SPETTRO FFT LIVE & PEAK HOLD", "dB");

    juce::Graphics::ScopedSaveState saveState(g);
    g.reduceClipRegion(plot.toNearestInt());

    juce::Path tracciaFFT, tracciaPeak;
    bool primo = true;
    int larghezzaPixel = static_cast<int>(plot.getWidth());

    for (int x = 0; x < larghezzaPixel; ++x)
    {
        float normX = static_cast<float>(x) / static_cast<float>(larghezzaPixel);
        float freqHz = 20.0f * std::pow(20000.0f / 20.0f, normX);

        int binIdx = static_cast<int>((freqHz / (sampleRateRiferimento / 2.0f)) * (dimensioneFFT / 2));
        binIdx = juce::jlimit(0, (dimensioneFFT / 2) - 1, binIdx);

        float valFFT = livelloSpettroIstantaneo[binIdx];
        float valPeak = tracciaPicchiMassimi[binIdx];

        float xPos = plot.getX() + static_cast<float>(x);
        float yPos = juce::jlimit(plot.getY(), plot.getBottom(), plot.getBottom() - (valFFT * plot.getHeight()));
        float yPeakPos = juce::jlimit(plot.getY(), plot.getBottom(), plot.getBottom() - (valPeak * plot.getHeight()));

        if (primo)
        {
            tracciaFFT.startNewSubPath(xPos, yPos);
            tracciaPeak.startNewSubPath(xPos, yPeakPos);
            primo = false;
        }
        else
        {
            tracciaFFT.lineTo(xPos, yPos);
            tracciaPeak.lineTo(xPos, yPeakPos);
        }
    }

    g.setColour(juce::Colour(0x88FF4081));
    g.strokePath(tracciaPeak, juce::PathStrokeType(1.0f));

    g.setColour(juce::Colour(0xffE040FB));
    g.strokePath(tracciaFFT, juce::PathStrokeType(1.6f));
}

void VisualizzatoreSpettro::disegnaCurvaEQLive(juce::Graphics& g, juce::Rectangle<float> area)
{
    auto plot = disegnaGrigliaEAssi(g, area, "3. RISPOSTA EQ TEORICA |H(e^jw)|", "dB");

    juce::Graphics::ScopedSaveState saveState(g);
    g.reduceClipRegion(plot.toNearestInt());

    juce::Path tracciaEQ;
    bool primo = true;
    int larghezzaPixel = static_cast<int>(plot.getWidth());

    for (int x = 0; x < larghezzaPixel; ++x)
    {
        float normX = static_cast<float>(x) / static_cast<float>(larghezzaPixel);
        float freqHz = 20.0f * std::pow(20000.0f / 20.0f, normX);

        float guadagno_dB = filtroLowRif.calcolaRispostaInFrequenza_dB(freqHz, sampleRateRiferimento)
                          + filtroMidRif.calcolaRispostaInFrequenza_dB(freqHz, sampleRateRiferimento)
                          + filtroHighRif.calcolaRispostaInFrequenza_dB(freqHz, sampleRateRiferimento);

        float xPos = plot.getX() + static_cast<float>(x);
        float rawY = plot.getCentreY() - (guadagno_dB / 24.0f) * (plot.getHeight() * 0.5f);
        float yPos = juce::jlimit(plot.getY(), plot.getBottom(), rawY);

        if (primo) { tracciaEQ.startNewSubPath(xPos, yPos); primo = false; }
        else { tracciaEQ.lineTo(xPos, yPos); }
    }

    g.setColour(juce::Colour(0xff00E5FF));
    g.strokePath(tracciaEQ, juce::PathStrokeType(2.0f));
}

void VisualizzatoreSpettro::disegnaSaturazioneLive(juce::Graphics& g, juce::Rectangle<float> area)
{
    auto plot = disegnaGrigliaEAssi(g, area, "4. SATURAZIONE TIMBRO tanh(x * Drive)", "Out");

    juce::Graphics::ScopedSaveState saveState(g);
    g.reduceClipRegion(plot.toNearestInt());

    juce::Path tracciaTanh;
    bool primo = true;
    int larghezzaPixel = static_cast<int>(plot.getWidth());

    // 1. Disegno della Curva di Trasferimento reattiva alla manopola DRIVE
    for (int x = 0; x < larghezzaPixel; ++x)
    {
        float normX = static_cast<float>(x) / static_cast<float>(larghezzaPixel);
        float inVal = juce::jmap(normX, 0.0f, 1.0f, -1.5f, 1.5f);
        float outVal = std::tanh(inVal * valoreDriveRiferimento);

        float xPos = plot.getX() + static_cast<float>(x);
        float rawY = plot.getY() + juce::jmap(outVal, -1.0f, 1.0f, 1.0f, 0.0f) * plot.getHeight();
        float yPos = juce::jlimit(plot.getY(), plot.getBottom(), rawY);

        if (primo) { tracciaTanh.startNewSubPath(xPos, yPos); primo = false; }
        else { tracciaTanh.lineTo(xPos, yPos); }
    }

    g.setColour(juce::Colour(0xffFF1744));
    g.strokePath(tracciaTanh, juce::PathStrokeType(1.8f));

    // 2. Indicatore Dinamico del Punto Operativo del Suono (Pallino Luminoso)
    float xPointNorm = juce::jmap(ampiezzaPiccoIstantaneo, 0.0f, 1.0f, 0.5f, 0.95f);
    float inValPoint = juce::jmap(xPointNorm, 0.0f, 1.0f, -1.5f, 1.5f);
    float outValPoint = std::tanh(inValPoint * valoreDriveRiferimento);

    float posXDot = plot.getX() + xPointNorm * plot.getWidth();
    float posYDot = juce::jlimit(plot.getY(), plot.getBottom(), plot.getY() + juce::jmap(outValPoint, -1.0f, 1.0f, 1.0f, 0.0f) * plot.getHeight());

    g.setColour(juce::Colour(0xffFFD700));
    g.fillEllipse(posXDot - 4.0f, posYDot - 4.0f, 8.0f, 8.0f);
    g.setColour(juce::Colours::white);
    g.drawEllipse(posXDot - 4.0f, posYDot - 4.0f, 8.0f, 8.0f, 1.2f);
}

void VisualizzatoreSpettro::paint(juce::Graphics& g)
{
    auto areaTotale = getLocalBounds().toFloat();
    float gappedW = (areaTotale.getWidth() - 10.0f) / 2.0f;
    float gappedH = (areaTotale.getHeight() - 10.0f) / 2.0f;

    juce::Rectangle<float> box1 (areaTotale.getX(), areaTotale.getY(), gappedW, gappedH);
    juce::Rectangle<float> box2 (areaTotale.getX() + gappedW + 10.0f, areaTotale.getY(), gappedW, gappedH);
    juce::Rectangle<float> box3 (areaTotale.getX(), areaTotale.getY() + gappedH + 10.0f, gappedW, gappedH);
    juce::Rectangle<float> box4 (areaTotale.getX() + gappedW + 10.0f, areaTotale.getY() + gappedH + 10.0f, gappedW, gappedH);

    disegnaOscilloscopioLive(g, box1);
    disegnaSpettroFFTLive(g, box2);
    disegnaCurvaEQLive(g, box3);
    disegnaSaturazioneLive(g, box4);
}