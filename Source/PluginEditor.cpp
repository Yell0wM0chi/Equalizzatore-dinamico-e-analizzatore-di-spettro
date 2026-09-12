/*
  ==============================================================================
    PluginEditor.cpp
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel(&stileCustom);

    addAndMakeVisible(visualizzatoreSpettro);

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

    configuraManopola(manopolaGuadagnoBassi,   "lowGain",   " dB");
    configuraManopola(manopolaFrequenzaBassi,  "lowFreq",   " Hz");
    configuraManopola(manopolaGuadagnoMedi,    "midGain",   " dB");
    configuraManopola(manopolaFrequenzaMedi,   "midFreq",   " Hz");
    configuraManopola(manopolaFattoreQMedi,    "midQ",      "");
    configuraManopola(manopolaGuadagnoAlti,    "highGain",  " dB");
    configuraManopola(manopolaFrequenzaAlti,   "highFreq",  " Hz");
    configuraManopola(manopolaSaturazioneDrive, "drive",     "x");
    configuraManopola(manopolaSogliaDinamica,  "threshold", " dB");

    startTimerHz(60);
    setSize (920, 590);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void AudioPluginAudioProcessorEditor::configuraManopola(juce::Slider& manopola, const juce::String& idParametro, const juce::String& suffissoUnita)
{
    manopola.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    manopola.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 65, 18);
    manopola.setTextValueSuffix(suffissoUnita);
    manopola.setMouseDragSensitivity(150);
    addAndMakeVisible(manopola);

    collegamentiParametri.push_back(
        std::make_unique<SliderAttachment>(audioProcessor.alberoParametriPlugin, idParametro, manopola));
}

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

void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1A1D24));

    g.setColour(juce::Colour(0xff0D1015));
    g.fillRect(0, 0, getWidth(), 35);
    
    g.setColour(juce::Colour(0xff00E5FF));
    g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    g.drawText("DSP AUDIO ANALYZER & DYNAMIC EQUALIZER", 15, 8, 450, 20, juce::Justification::left);

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

    disegnaCardComandi(juce::Rectangle<float>(15,  370, 185, 205), "LOW BAND",          juce::Colour(0xffFFD700));
    disegnaCardComandi(juce::Rectangle<float>(210, 370, 265, 205), "MID BAND (DYNAMIC)", juce::Colour(0xff00E5FF));
    disegnaCardComandi(juce::Rectangle<float>(485, 370, 185, 205), "HIGH BAND",         juce::Colour(0xffFF4081));
    disegnaCardComandi(juce::Rectangle<float>(680, 370, 225, 205), "PROCESSING",        juce::Colours::white);

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