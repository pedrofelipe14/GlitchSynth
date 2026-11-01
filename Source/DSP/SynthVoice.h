#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

enum class OscWaveform
{
    sine = 0,
    saw,
    square,
    triangle
};

/** A simple polyphonic voice: one band-limited-ish oscillator, an amplitude ADSR,
    and a resonant low-pass filter with ADSR-driven cutoff modulation. */
class GlitchSynthVoice : public juce::SynthesiserVoice
{
public:
    GlitchSynthVoice();

    bool canPlaySound(juce::SynthesiserSound*) override;

    void startNote(int midiNoteNumber, float velocity,
                    juce::SynthesiserSound*, int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;

    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                          int startSample, int numSamples) override;

    void prepare(double sampleRate, int samplesPerBlock);

    // Live parameters, set from the processor before each block.
    void setWaveform(OscWaveform newWaveform) { waveform = newWaveform; }
    void setAmpAdsrParams(const juce::ADSR::Parameters& p) { ampAdsr.setParameters(p); }
    void setFilterParams(float cutoffHz, float resonance, float envAmountSemitones)
    {
        filterCutoffHz = cutoffHz;
        filterResonance = resonance;
        filterEnvAmount = envAmountSemitones;
    }

private:
    float renderOscSample();

    OscWaveform waveform = OscWaveform::saw;

    double currentSampleRate = 44100.0;
    double phase = 0.0;
    double phaseIncrement = 0.0;
    float level = 0.0f;

    juce::ADSR ampAdsr;
    juce::dsp::StateVariableTPTFilter<float> filter;

    float filterCutoffHz = 4000.0f;
    float filterResonance = 0.7f;
    float filterEnvAmount = 24.0f;

    juce::Random random;
};

class GlitchSynthSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};
