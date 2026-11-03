#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

/** The "wet" channel: an ambience processor with three modes.
    - Reverb: a lush stereo room/hall. Time = size, Modify = brightness.
    - Delay:  a tape-ish stereo echo. Time = delay time, Modify = feedback.
    - Slip:   a pitch-slipping delay whose repeats play backwards or at another
              octave. Time = slice length, Modify = speed (reverse / half / reverse
              octave-up / octave-up). */
class WetEngine
{
public:
    enum class Mode { reverb = 0, delay, slip };

    struct Params
    {
        Mode mode = Mode::reverb;
        float mix = 0.0f;    // 0 = dry, 1 = fully wet
        float time = 0.5f;   // 0..1, meaning depends on mode
        float modify = 0.5f; // 0..1, meaning depends on mode
    };

    void prepare(double sampleRate, int numChannels, int maximumBlockSize);
    void reset();
    void setParams(const Params& newParams) { params = newParams; }

    /** Processes buffer in place. */
    void process(juce::AudioBuffer<float>& buffer);

private:
    float processDelaySample(int channel, float input, float delaySamples, float feedback);
    float processSlipSample(int channel, float input, float sliceSamples, double speed);
    float readRing(int channel, double delaySamples) const;

    double sampleRate = 44100.0;
    int numChannelsPrepared = 0;
    Params params;

    juce::dsp::Reverb reverb;
    juce::AudioBuffer<float> wetBuffer;

    juce::AudioBuffer<float> ring;
    int ringLength = 1;
    int writePos = 0;
    std::vector<float> feedbackLp;
    std::vector<float> lastWet;

    double slipPhase = 0.0;

    juce::SmoothedValue<float> smoothedMix;
    juce::SmoothedValue<float> smoothedDelay;
    Mode lastMode = Mode::reverb;
};
