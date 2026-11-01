#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

/** Audio-rate glitch processor: bitcrushing, tempo-synced stutter/repeat,
    and chunk reversal. Designed to run on either an external audio
    sidechain or the synth's own output. */
class GlitchEngine
{
public:
    void prepare(double sampleRate, int numChannels, int maximumBlockSize);
    void reset();

    struct Params
    {
        float mix = 1.0f;              // 0 = dry, 1 = fully glitched
        float bitDepth = 16.0f;        // 1-16 bits
        float sampleRateDivider = 1.0f;// 1 = no reduction, >1 = crushed
        float stutterRateHz = 8.0f;    // retrigger frequency for stutter/repeat
        float stutterAmount = 0.0f;    // 0 = never stutters, 1 = always
        float reverseAmount = 0.0f;    // 0 = never reverses, 1 = always
    };

    void setParams(const Params& newParams) { params = newParams; }

    /** Processes buffer in place. */
    void process(juce::AudioBuffer<float>& buffer);

private:
    void refreshChunkIfNeeded(int numChannels, int chunkLenSamples);
    float applyBitcrush(int channel, float sample);

    double sampleRate = 44100.0;
    Params params;

    // Bitcrush / sample-and-hold state, per channel.
    std::vector<float> heldSample;
    std::vector<int> holdCounter;

    // Stutter/repeat + reverse chunk buffer.
    juce::AudioBuffer<float> chunkBuffer;
    int chunkLengthSamples = 0;
    int chunkWritePos = 0;
    int chunkReadPos = 0;
    bool chunkIsCaptured = false;
    bool chunkIsReversed = false;
    int samplesUntilNextDecision = 0;

    juce::Random random;
};
