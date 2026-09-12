/*
  ==============================================================================
    PluginEditor.cpp
    ----------------------------------------------------------------------------
    Questo file si occupa di costruire concretamente la finestra del plugin,
    posizionando manopole, pulsanti e collegandoli al motore audio.
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel(&stileCustom); // Applica lo stile estetico custom

    addAndMakeVisible(visualizzatoreSpettro); // Rende visibili i grafici

    // --- PULSANTE PER CARICARE IL FILE AUDIO ---
    pulsanteCaricaFile.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2A323D));
    pulsanteCaricaFile.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    pulsanteCaricaFile.onClick = [this]()
    {
        selettoreFile = std::make_unique<juce::FileChooser>("Seleziona File Audio...",
                                                            juce::File::getSpecialLocation(juce::File::userHomeDirectory),
                                                            "*.wav;*.mp3;*.aiff;*.flac");

        auto modalita = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
        selettoreFile->launchAsync(modalita, [this](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file.exists())
            {
                audioProcessor.gestoreFileAudio.caricaFileAudioDaDisco(file);
                pulsantePlayStop.setButtonText("STOP");
                pulsantePlayStop.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffFF1744));
            }
        });
    };
    addAndMakeVisible(pulsanteCaricaFile);

    // --- PULSANTE DI PLAY / PAUSA ---
    pulsantePlayStop.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff00E676));
    pulsantePlayStop.setColour(juce::TextButton::textColourOffId, juce::Colours::black);
    pulsantePlayStop.onClick = [this]()
    {
        bool stato = audioProcessor.gestoreFileAudio.isRiproduzioneAttiva();
        audioProcessor.gestoreFileAudio.impostaRiproduzioneAttiva(!stato);
        pulsantePlayStop.setButtonText(!stato ? "STOP" : "RUN");
        pulsantePlayStop.setColour(juce::TextButton::buttonColourId, !stato ? juce::Colour(0xffFF1744) : juce::Colour(0xff00E676));
    };
    addAndMakeVisible(pulsantePlayStop);

    // --- CONFIGURAZIONE E COLLEGAMENTO DI TUTTE LE MANOPOLE ---
    configuraManopola(manopolaGuadagnoBassi,   "lowGain",   " dB");
    configuraManopola(manopolaFrequenzaBassi,  "lowFreq",   " Hz");
    configuraManopola(manopolaGuadagnoMedi,    "midGain",   " dB");
    configuraManopola(manopolaFrequenzaMedi,   "midFreq",   " Hz");
    configuraManopola(manopolaFattoreQMedi,    "midQ",      "");
    configuraManopola(manopolaGuadagnoAlti,    "highGain",  " dB");
    configuraManopola(manopolaFrequenzaAlti,   "highFreq",  " Hz");
    configuraManopola(manopolaSaturazioneDrive, "drive",     "x");
    configuraManopola(manopolaSogliaDinamica,  "threshold", " dB");

    startTimerHz(60); // Frequenza di aggiornamento schermo (60 FPS)
    setSize (920, 590); // Dimensioni finestra
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

// Funzione helper per evitare ripetizioni durante l'impostazione delle manopole
void AudioPluginAudioProcessorEditor::configuraManopola(juce::Slider& manopola, const juce::String& idParametro, const juce::String& suffissoUnita)
{
    manopola.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    manopola.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 65, 18);
    manopola.setTextValueSuffix(suffissoUnita);
    manopola.setMouseDragSensitivity(150);
    addAndMakeVisible(manopola);

    // Collega automaticamente la manopola visiva al relativo parametro dell'engine audio
    collegamentiParametri.push_back(
        std::make_unique<SliderAttachment>(audioProcessor.alberoParametriPlugin, idParametro, manopola));
}

// Timer a 60Hz per inviare i dati audio alla grafica
void AudioPluginAudioProcessorEditor::timerCallback()
{
    if (audioProcessor.bloccoAudioProntoPerGUI)
    {
        float valoreDriveAttuale = audioProcessor.alberoParametriPlugin.getRawParameterValue("drive")->load();

        visualizzatoreSpettro.riceviCampioniAudioPerFFT(audioProcessor.bufferFifoPerGUI, AudioPluginAudioProcessor::dimensioneFFT);
        visualizzatoreSpettro.impostaFiltriDiRiferimento(
            audioProcessor.filtroLowRef,
            audioProcessor.filtroMidRef,
            audioProcessor.filtroHighRef,
            48000.0f,
            valoreDriveAttuale
        );
        audioProcessor.bloccoAudioProntoPerGUI = false;
    }
}

// Disegna lo sfondo e i riquadri contenitivi colorati per le varie sezioni
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1A1D24)); // Sfondo scuro

    // Barra del titolo
    g.setColour(juce::Colour(0xff0D1015));
    g.fillRect(0, 0, getWidth(), 35);
    
    g.setColour(juce::Colour(0xff00E5FF));
    g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    g.drawText("DSP AUDIO ANALYZER & DYNAMIC EQUALIZER", 15, 8, 450, 20, juce::Justification::left);

    // Funzione per disegnare le schede contenitore
    auto disegnaCardComandi = [&](juce::Rectangle<float> area, juce::String titolo, juce::Colour colore)
    {
        g.setColour(juce::Colour(0xff12161F));
        g.fillRoundedRectangle(area, 8.0f);

        g.setColour(colore.withAlpha(0.6f));
        g.drawRoundedRectangle(area, 8.0f, 1.2f);

        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.setColour(colore);
        g.drawText(titolo, static_cast<int>(area.getX() + 10), static_cast<int>(area.getY() + 6), static_cast<int>(area.getWidth() - 20), 15, juce::Justification::centred);
    };

    // Quattro schede di controllo
    disegnaCardComandi(juce::Rectangle<float>(15,  370, 185, 205), "LOW BAND",          juce::Colour(0xffFFD700));
    disegnaCardComandi(juce::Rectangle<float>(210, 370, 265, 205), "MID BAND (DYNAMIC)", juce::Colour(0xff00E5FF));
    disegnaCardComandi(juce::Rectangle<float>(485, 370, 185, 205), "HIGH BAND",         juce::Colour(0xffFF4081));
    disegnaCardComandi(juce::Rectangle<float>(680, 370, 225, 205), "PROCESSING",        juce::Colours::white);

    // Testi ed etichette delle manopole
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xffA0ABBA));

    int YEtichette = 370 + 130;

    g.drawText("LOW GAIN",   30,  YEtichette, 75, 15, juce::Justification::centred);
    g.drawText("LOW FREQ",   110, YEtichette, 75, 15, juce::Justification::centred);

    g.drawText("MID GAIN",   225, YEtichette, 75, 15, juce::Justification::centred);
    g.drawText("MID FREQ",   305, YEtichette, 75, 15, juce::Justification::centred);
    g.drawText("MID Q",      385, YEtichette, 75, 15, juce::Justification::centred);

    g.drawText("HIGH GAIN",  500, YEtichette, 75, 15, juce::Justification::centred);
    g.drawText("HIGH FREQ",  580, YEtichette, 75, 15, juce::Justification::centred);

    g.drawText("DRIVE",      700, YEtichette, 85, 15, juce::Justification::centred);
    g.drawText("THRESH",     800, YEtichette, 85, 15, juce::Justification::centred);
}

// Posizionamento esatto in pixel di ogni elemento della GUI
void AudioPluginAudioProcessorEditor::resized()
{
    pulsanteCaricaFile.setBounds(630, 6, 150, 22);
    pulsantePlayStop.setBounds(790, 6, 110, 22);

    visualizzatoreSpettro.setBounds(15, 45, 890, 310);

    int YPannelloControlli = 370;
    int larghezzaManopola = 75;
    int altezzaManopola = 95;

    manopolaGuadagnoBassi.setBounds(30, YPannelloControlli + 30, larghezzaManopola, altezzaManopola);
    manopolaFrequenzaBassi.setBounds(110, YPannelloControlli + 30, larghezzaManopola, altezzaManopola);

    manopolaGuadagnoMedi.setBounds(225, YPannelloControlli + 30, larghezzaManopola, altezzaManopola);
    manopolaFrequenzaMedi.setBounds(305, YPannelloControlli + 30, larghezzaManopola, altezzaManopola);
    manopolaFattoreQMedi.setBounds(385, YPannelloControlli + 30, larghezzaManopola, altezzaManopola);

    manopolaGuadagnoAlti.setBounds(500, YPannelloControlli + 30, larghezzaManopola, altezzaManopola);
    manopolaFrequenzaAlti.setBounds(580, YPannelloControlli + 30, larghezzaManopola, altezzaManopola);

    manopolaSaturazioneDrive.setBounds(700, YPannelloControlli + 30, 85, altezzaManopola);
    manopolaSogliaDinamica.setBounds(800, YPannelloControlli + 30, 85, altezzaManopola);
}