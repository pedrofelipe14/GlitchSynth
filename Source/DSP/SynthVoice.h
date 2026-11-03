#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

#include "CustomWave.h"

enum class OscWaveform
{
    sine = 0,
    saw,
    square,
    triangle,
    custom
};

enum class LfoShape
{
    sine = 0,
    triangle,
    saw,
    square,
    random // sample & hold: a new random level every cycle
};

enum class LfoTarget
{
    filter = 0,
    pitch,
    volume
};

/** The textbook shape of a waveform, without anti-aliasing, phase in [0, 1). Used to
    display it, and as the starting point when you begin shaping your own. */
float naiveWaveformSample(OscWaveform waveform, float phase);

/** A polyphonic voice: two oscillators (preset shapes or a custom point-shaped cycle) mixed
    together, an amplitude ADSR, a resonant low-pass filter with ADSR-driven
    cutoff modulation, and an LFO that restarts with each note. */
class GlitchSynthVoice : public juce::SynthesiserVoice
{
public:
    static constexpr int numOscillators = 2;

    struct OscSettings
    {
        OscWaveform waveform = OscWaveform::saw;
        float level = 1.0f;
        float pitchOffsetSemitones = 0.0f; // octave + semitones + fine tune, combined
        const CustomWave* customWave = nullptr;
    };

    struct LfoSettings
    {
        LfoShape shape = LfoShape::sine;
        LfoTarget target = LfoTarget::filter;
        float rateHz = 4.0f;
        float depth = 0.0f; // 0..1
    };

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
    void setOscillator(int index, const OscSettings& settings) { oscSettings[(size_t) index] = settings; }
    void setAmpAdsrParams(const juce::ADSR::Parameters& p) { ampAdsr.setParameters(p); }
    void setFilterParams(float cutoffHz, float resonance, float envAmountSemitones)
    {
        filterCutoffHz = cutoffHz;
        filterResonance = resonance;
        filterEnvAmount = envAmountSemitones;
    }
    void setPitchBendRange(float semitones) { pitchBendRange = semitones; }
    void setLfo(const LfoSettings& settings) { lfoSettings = settings; }

private:
    struct OscState
    {
        double phase = 0.0;
        double phaseIncrement = 0.0;
    };

    float renderOscSample(int index);

    /** Advances the LFO by one sample and returns its smoothed value, -1..1. */
    float nextLfoSample();

    std::array<OscSettings, numOscillators> oscSettings;
    std::array<OscState, numOscillators> oscStates;

    double currentSampleRate = 44100.0;
    int currentNote = 60;
    float level = 0.0f;

    // Pitch wheel (also how FL Studio sends slide notes): 0..16383, 8192 = centre.
    int pitchWheelPosition = 8192;
    float pitchBendRange = 12.0f;
    float currentBend = 0.0f; // semitones, smoothed toward the wheel position

    LfoSettings lfoSettings;
    double lfoPhase = 0.0;
    float lfoHeldValue = 0.0f; // current step of the random shape
    float lfoSmoothed = 0.0f;  // light smoothing so square/random steps don't click
    float lfoSmoothCoeff = 0.99f;

    // Custom waves can sit off-centre; this one-pole high-pass removes that offset (DC),
    // which would otherwise thump on every note and eat headroom.
    float dcBlockCoeff = 0.999f;
    float dcBlockLastIn = 0.0f;
    float dcBlockLastOut = 0.0f;

    // Last output sample, and a decaying copy of it added after a hard voice steal
    // so the waveform never jumps (that jump is an audible click).
    float lastOutput = 0.0f;
    float declickValue = 0.0f;

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
