#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

/** A Chase Bliss "Mood"/"Blooper"-style granular looping degrader.
    Continuously records live audio, periodically freezes a grain and loops it,
    pitch-shifts the loop, progressively degrades it the longer it repeats, and
    lets a single "chaos" control introduce generative randomness: tape-style
    wow/flutter, random reverse flips, and random decisions to grab fresh
    material vs. keep looping (and degrading) what it already has. */
class GlitchEngine
{
public:
    void prepare(double sampleRate, int numChannels, int maximumBlockSize);
    void reset();

    struct Params
    {
        float mix = 1.0f;            // 0 = dry, 1 = fully glitched
        float grainSizeMs = 250.0f;  // length of the captured loop/grain
        float pitchRatio = 1.0f;     // playback speed/pitch of the loop (0.25-4x)
        float holdAmount = 0.0f;     // 0 = keep grabbing fresh grains, 1 = freeze and loop forever
        float degrade = 0.0f;        // bit/rate crush + grit, intensified the longer a grain repeats
        float chaos = 0.0f;          // generative randomness: wow/flutter, random reverse flips, drift
        float reverseAmount = 0.0f;  // base chance a freshly captured grain plays backwards
        float toneTilt = 0.5f;       // 0 = dark/lowpassed, 1 = bright/highpass-emphasised
    };

    void setParams(const Params& newParams) { params = newParams; }

    /** Processes buffer in place. */
    void process(juce::AudioBuffer<float>& buffer);

private:
    void captureNewGrain(int numChannels);
    void onGrainCycleComplete(int numChannels);
    float applyDegrade(int channel, float sample);
    float applyToneTilt(int channel, float sample);
    float nextWowOffset();

    double sampleRate = 44100.0;
    Params params;

    // Continuous live-input recording ring; source material for fresh grain captures.
    juce::AudioBuffer<float> liveRing;
    int liveRingLengthSamples = 1;
    int liveWritePos = 0;

    // Snapshot of the grain currently looping.
    juce::AudioBuffer<float> grainSnapshot;
    int grainLengthSamples = 1;
    double playbackPos = 0.0;
    bool playingReversed = false;
    int repeatCount = 0;

    // Per-channel degrade (sample-and-hold crush) state.
    std::vector<float> heldSample;
    std::vector<int> holdCounter;

    // Per-channel tone-tilt one-pole filter state.
    std::vector<float> tiltLpState;

    // Tape-style wow/flutter random-walk LFO, shared across channels.
    float wowCurrent = 0.0f;
    float wowTarget = 0.0f;
    int wowCounter = 0;

    juce::Random random;
};
