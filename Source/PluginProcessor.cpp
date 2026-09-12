/*
  ==============================================================================
    PluginProcessor.cpp
    ----------------------------------------------------------------------------
    In questo file c'è il "motore" del suono: legge le manopole, applica i 3 filtri
    (bassi, medi, alti), applica l'effetto dinamico sui medi e la saturazione finale.
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

AudioPluginAudioProcessor::AudioPluginAudioProcessor()
     : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
       alberoParametriPlugin(*this, nullptr, "ParametriDSP", creaParametriIniziali())
{
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor() {}

// Definizione di tutte le manopole disponibili e dei relativi intervalli numerici
juce::AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::creaParametriIniziali()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> lista;

    // Manopole Bassi
    lista.push_back(std::make_unique<juce::AudioParameterFloat>("lowGain", "Guadagno Bassi", -24.0f, 24.0f, 0.0f));
    lista.push_back(std::make_unique<juce::AudioParameterFloat>("lowFreq", "Frequenza Bassi", juce::NormalisableRange<float>(20.0f, 500.0f, 0.1f, 0.4f), 100.0f));

    // Manopole Medi (Dinamici)
    lista.push_back(std::make_unique<juce::AudioParameterFloat>("midGain", "Guadagno Medi", -24.0f, 24.0f, 0.0f));
    lista.push_back(std::make_unique<juce::AudioParameterFloat>("midFreq", "Frequenza Medi", juce::NormalisableRange<float>(200.0f, 5000.0f, 1.0f, 0.4f), 1000.0f));
    lista.push_back(std::make_unique<juce::AudioParameterFloat>("midQ", "Q Medi", juce::NormalisableRange<float>(0.1f, 10.0f, 0.05f, 0.5f), 0.707f));

    // Manopole Alti
    lista.push_back(std::make_unique<juce::AudioParameterFloat>("highGain", "Guadagno Alti", -24.0f, 24.0f, 0.0f));
    lista.push_back(std::make_unique<juce::AudioParameterFloat>("highFreq", "Frequenza Alti", juce::NormalisableRange<float>(1000.0f, 20000.0f, 10.0f, 0.4f), 5000.0f));

    // Manopole Saturazione e Soglia Dinamica
    lista.push_back(std::make_unique<juce::AudioParameterFloat>("drive", "Saturazione Drive", 1.0f, 10.0f, 1.0f));
    lista.push_back(std::make_unique<juce::AudioParameterFloat>("threshold", "Soglia Dinamica", -60.0f, 0.0f, -20.0f));

    return { lista.begin(), lista.end() };
}

// Preparazione iniziale prima di far partire l'audio
void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sampleRateDAW = sampleRate;

    // Resetta lo stato di memoria di tutti i filtri
    filtroBassiL.azzeraMemoriaFiltro(); filtroBassiR.azzeraMemoriaFiltro();
    filtroMediL.azzeraMemoriaFiltro();  filtroMediR.azzeraMemoriaFiltro();
    filtroAltiL.azzeraMemoriaFiltro();  filtroAltiR.azzeraMemoriaFiltro();
}

void AudioPluginAudioProcessor::releaseResources() {}

// QUESTA È LA FUNZIONE PRINCIPALE CHE ELABORA L'AUDIO AD OGNI ISTANTE
void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& bufferAudio, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    bufferAudio.clear(); // Pulisce il contenitore dell'audio

    // Se siamo in STOP, non fa nulla
    if (!gestoreFileAudio.isRiproduzioneAttiva())
        return;

    int numeroCampioniBlocco = bufferAudio.getNumSamples();
    int numeroCanaliAudio = bufferAudio.getNumChannels();

    // 1. Legge l'audio dal file e lo inserisce nel buffer
    for (int s = 0; s < numeroCampioniBlocco; ++s)
    {
        for (int ch = 0; ch < numeroCanaliAudio; ++ch)
        {
            float campioneLetto = gestoreFileAudio.leggiCampioneConInterpolazione(ch, sampleRateDAW);
            bufferAudio.setSample(ch, s, campioneLetto);
        }
        gestoreFileAudio.avanzaPosizioneLettura(sampleRateDAW);
    }

    // 2. Legge i valori attuali di tutte le manopole impostate dall'utente
    float gBassi = alberoParametriPlugin.getRawParameterValue("lowGain")->load();
    float fBassi = alberoParametriPlugin.getRawParameterValue("lowFreq")->load();
    
    float gMedi  = alberoParametriPlugin.getRawParameterValue("midGain")->load();
    float fMedi  = alberoParametriPlugin.getRawParameterValue("midFreq")->load();
    float qMedi  = alberoParametriPlugin.getRawParameterValue("midQ")->load();
    
    float gAlti  = alberoParametriPlugin.getRawParameterValue("highGain")->load();
    float fAlti  = alberoParametriPlugin.getRawParameterValue("highFreq")->load();
    
    float spintaDrive = alberoParametriPlugin.getRawParameterValue("drive")->load();
    float sogliaDinamica_dB = alberoParametriPlugin.getRawParameterValue("threshold")->load();

    // Aggiorna i filtri dei bassi e degli alti
    filtroBassiL.impostaLowShelf((float)sampleRateDAW, fBassi, gBassi);
    filtroBassiR.impostaLowShelf((float)sampleRateDAW, fBassi, gBassi);

    filtroAltiL.impostaHighShelf((float)sampleRateDAW, fAlti, gAlti);
    filtroAltiR.impostaHighShelf((float)sampleRateDAW, fAlti, gAlti);

    // Mantiene una copia aggiornata per permettere al grafico di tracciare la linea EQ
    filtroLowRef = filtroBassiL;
    filtroHighRef = filtroAltiL;

    auto* canaleSinistro = bufferAudio.getWritePointer(0);
    auto* canaleDestro = bufferAudio.getNumChannels() > 1 ? bufferAudio.getWritePointer(1) : nullptr;

    // 3. Applica il filtraggio e l'effetto dinamico campione per campione
    for (int i = 0; i < numeroCampioniBlocco; ++i)
    {
        // Segue l'inviluppo di volume del canale sinistro
        float ampiezzaAssoluta = std::abs(canaleSinistro[i]);
        inviluppoLivelloMedi = 0.999f * inviluppoLivelloMedi + 0.001f * ampiezzaAssoluta;
        float inviluppo_dB = juce::Decibels::gainToDecibels(inviluppoLivelloMedi + 1e-5f);

        // LOGICA DINAMICA: se il volume supera la soglia (threshold), abbassa i medi automaticamente
        float gMediEffettivo = gMedi;
        if (inviluppo_dB > sogliaDinamica_dB)
            gMediEffettivo -= (inviluppo_dB - sogliaDinamica_dB) * 0.5f;

        // Imposta il filtro medi con il guadagno dinamico calcolato
        filtroMediL.impostaPeak((float)sampleRateDAW, fMedi, gMediEffettivo, qMedi);
        filtroMediR.impostaPeak((float)sampleRateDAW, fMedi, gMediEffettivo, qMedi);
        filtroMidRef = filtroMediL;

        // --- FILTRAGGIO E SATURAZIONE SUL CANALE SINISTRO ---
        float sL = filtroBassiL.elaboraCampione(canaleSinistro[i]);
        sL = filtroMediL.elaboraCampione(sL);
        sL = filtroAltiL.elaboraCampione(sL);
        sL = std::tanh(sL * spintaDrive); // Applicazione saturazione valvolare (tangente iperbolica)
        canaleSinistro[i] = sL;

        // --- FILTRAGGIO E SATURAZIONE SUL CANALE DESTRO ---
        if (canaleDestro != nullptr)
        {
            float sR = filtroBassiR.elaboraCampione(canaleDestro[i]);
            sR = filtroMediR.elaboraCampione(sR);
            sR = filtroAltiR.elaboraCampione(sR);
            sR = std::tanh(sR * spintaDrive);
            canaleDestro[i] = sR;
        }

        // --- TRASFERIMENTO DATI ALLA GRAFICA ---
        if (indiceBufferFifo == dimensioneFFT)
        {
            bloccoAudioProntoPerGUI = true;
            indiceBufferFifo = 0;
        }
        bufferFifoPerGUI[indiceBufferFifo++] = sL;
    }
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AudioPluginAudioProcessor(); }

// Salvataggio stato delle manopole
void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto stato = alberoParametriPlugin.copyState();
    std::unique_ptr<juce::XmlElement> xml (stato.createXml());
    copyXmlToBinary (*xml, destData);
}

// Ripristino stato delle manopole
void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (alberoParametriPlugin.state.getType()))
            alberoParametriPlugin.replaceState (juce::ValueTree::fromXml (*xmlState));
}