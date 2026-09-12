/*
  ==============================================================================
    PluginProcessor.h
    Progetto DSP - Engine Audio Principale
  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "FiltroBiquad.h"
#include "GestoreFileAudio.h"

class AudioPluginAudioProcessor  : public juce::AudioProcessor
{
public:
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    GestoreFileAudio gestoreFileAudio;
    
    static constexpr int dimensioneFFT = 1024;
    float bufferFifoPerGUI[dimensioneFFT];
    int indiceBufferFifo = 0;
    bool bloccoAudioProntoPerGUI = false;

    FiltroBiquad filtroLowRef, filtroMidRef, filtroHighRef;

    juce::AudioProcessorValueTreeState alberoParametriPlugin;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout creaParametriIniziali();

    FiltroBiquad filtroBassiL, filtroBassiR;
    FiltroBiquad filtroMediL,  filtroMediR;
    FiltroBiquad filtroAltiL,  filtroAltiR;

    float inviluppoLivelloMedi = 0.0f;
    double sampleRateDAW = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
