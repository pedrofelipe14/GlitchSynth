#include "GlitchEngine.h"

void GlitchEngine::prepare(double newSampleRate, int numChannels, int /*maximumBlockSize*/)
{
    sampleRate = newSampleRate;
    heldSample.assign((size_t) numChannels, 0.0f);
    holdCounter.assign((size_t) numChannels, 0);
    chunkBuffer.setSize(numChannels, 1);
    chunkLengthSamples = 1;
    reset();
}

void GlitchEngine::reset()
{
    chunkWritePos = 0;
    chunkReadPos = 0;
    chunkIsCaptured = false;
    chunkIsReversed = false;
    samplesUntilNextDecision = 0;
    std::fill(holdCounter.begin(), holdCounter.end(), 0);
    std::fill(heldSample.begin(), heldSample.end(), 0.0f);
}

void GlitchEngine::refreshChunkIfNeeded(int numChannels, int desiredChunkLenSamples)
{
    desiredChunkLenSamples = juce::jmax(1, desiredChunkLenSamples);

    if (chunkBuffer.getNumChannels() != numChannels
        || chunkLengthSamples != desiredChunkLenSamples)
    {
        chunkLengthSamples = desiredChunkLenSamples;
        chunkBuffer.setSize(numChannels, chunkLengthSamples, false, true, true);
        chunkBuffer.clear();
        chunkWritePos = 0;
        chunkReadPos = 0;
        chunkIsCaptured = false;
        samplesUntilNextDecision = 0;
    }
}

float GlitchEngine::applyBitcrush(int channel, float sample)
{
    if (holdCounter[(size_t) channel] <= 0)
    {
        heldSample[(size_t) channel] = sample;
        holdCounter[(size_t) channel] = juce::jmax(1, (int) std::round(params.sampleRateDivider));
    }
    --holdCounter[(size_t) channel];

    const float held = heldSample[(size_t) channel];
    const float levels = std::pow(2.0f, juce::jlimit(1.0f, 16.0f, params.bitDepth));
    return std::round(held * levels) / levels;
}

void GlitchEngine::process(juce::AudioBuffer<float>& buffer)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    const float safeStutterRateHz = juce::jlimit(1.0f, 32.0f, params.stutterRateHz);
    const int desiredChunkLen = (int) std::round(sampleRate / (double) safeStutterRateHz);
    refreshChunkIfNeeded(numChannels, desiredChunkLen);

    for (int i = 0; i < numSamples; ++i)
    {
        if (samplesUntilNextDecision <= 0)
        {
            samplesUntilNextDecision = chunkLengthSamples;

            if (random.nextFloat() < params.stutterAmount)
            {
                chunkIsCaptured = true;
                chunkIsReversed = random.nextFloat() < params.reverseAmount;
                chunkReadPos = chunkIsReversed ? chunkLengthSamples - 1 : 0;
            }
            else
            {
                chunkIsCaptured = false;
            }
        }

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* data = buffer.getWritePointer(ch);
            const float drySample = data[i];

            // Keep recording live audio so future stutters have fresh material to play back.
            chunkBuffer.setSample(ch, chunkWritePos, drySample);

            float wetSample = chunkIsCaptured ? chunkBuffer.getSample(ch, chunkReadPos)
                                               : drySample;
            wetSample = applyBitcrush(ch, wetSample);

            data[i] = drySample + params.mix * (wetSample - drySample);
        }

        chunkWritePos = (chunkWritePos + 1) % chunkLengthSamples;

        if (chunkIsCaptured)
        {
            chunkReadPos += chunkIsReversed ? -1 : 1;

            if (chunkReadPos >= chunkLengthSamples)
                chunkReadPos = 0;
            else if (chunkReadPos < 0)
                chunkReadPos = chunkLengthSamples - 1;
        }

        --samplesUntilNextDecision;
    }
}
