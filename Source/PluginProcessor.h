#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/SynthVoice.h"
#include "DSP/GlitchEngine.h"
#include "DSP/WetEngine.h"

class GlitchSynthAudioProcessor : public juce::AudioProcessor
{
public:
    GlitchSynthAudioProcessor();
    ~GlitchSynthAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgramIndex; }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;

    // One custom-shaped cycle per oscillator, edited from the UI and played by every voice.
    std::array<CustomWave, GlitchSynthVoice::numOscillators> customWaves;

private:
    static juce::String customWaveStateKey(int oscIndex);

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void updateVoiceParametersFromState();

    juce::Synthesiser synth;
    GlitchEngine sidechainGlitch;
    GlitchEngine selfGlitch;
    WetEngine wet;

    juce::AudioBuffer<float> sidechainWorkBuffer;

    int currentProgramIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlitchSynthAudioProcessor)
};
