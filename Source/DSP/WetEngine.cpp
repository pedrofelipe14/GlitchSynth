#include "WetEngine.h"

namespace
{
    constexpr double maxDelaySec = 2.0;

    /** Slip speed choices, picked by the Modify knob. Negative = backwards. */
    double slipSpeedFor(float modify)
    {
        if (modify < 0.25f) return -1.0; // reverse
        if (modify < 0.5f)  return 0.5;  // half speed, one octave down
        if (modify < 0.75f) return -2.0; // reverse, one octave up
        return 2.0;                      // double speed, one octave up
    }
}

void WetEngine::prepare(double newSampleRate, int numChannels, int maximumBlockSize)
{
    sampleRate = newSampleRate;
    numChannelsPrepared = numChannels;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) maximumBlockSize;
    spec.numChannels = (juce::uint32) numChannels;
    reverb.prepare(spec);

    wetBuffer.setSize(numChannels, maximumBlockSize);

    ringLength = (int) std::ceil(sampleRate * maxDelaySec * 2.0) + 4;
    ring.setSize(numChannels, ringLength);

    feedbackLp.assign((size_t) numChannels, 0.0f);
    lastWet.assign((size_t) numChannels, 0.0f);

    smoothedMix.reset(sampleRate, 0.05);
    smoothedDelay.reset(sampleRate, 0.25);

    reset();
}

void WetEngine::reset()
{
    reverb.reset();
    ring.clear();
    writePos = 0;
    slipPhase = 0.0;
    std::fill(feedbackLp.begin(), feedbackLp.end(), 0.0f);
    std::fill(lastWet.begin(), lastWet.end(), 0.0f);
    smoothedMix.setCurrentAndTargetValue(params.mix);
}

float WetEngine::readRing(int channel, double delaySamples) const
{
    double readPos = (double) writePos - delaySamples;
    while (readPos < 0.0)
        readPos += ringLength;

    const int i0 = (int) readPos % ringLength;
    const int i1 = (i0 + 1) % ringLength;
    const float frac = (float) (readPos - std::floor(readPos));

    const float s0 = ring.getSample(channel, i0);
    const float s1 = ring.getSample(channel, i1);
    return s0 + frac * (s1 - s0);
}

float WetEngine::processDelaySample(int channel, float input, float delaySamples, float feedback)
{
    // Ping-pong-ish width: the right channel runs slightly late.
    const float channelDelay = delaySamples * (channel == 1 ? 1.03f : 1.0f);
    const float delayed = readRing(channel, channelDelay);

    // Darken the repeats a little each time, like tape.
    auto& lp = feedbackLp[(size_t) channel];
    lp += 0.35f * (delayed - lp);

    ring.setSample(channel, writePos, input + std::tanh(lp * feedback));
    return delayed;
}

float WetEngine::processSlipSample(int channel, float input, float sliceSamples, double speed)
{
    // Two overlapping read heads with sin^2 windows (they sum to exactly 1).
    // Each head's delay sweeps linearly so it reads at `speed` relative to real time.
    const double sweep = std::abs(1.0 - speed) * (double) sliceSamples;
    float out = 0.0f;

    for (int h = 0; h < 2; ++h)
    {
        double p = slipPhase + 0.5 * h;
        if (p >= 1.0)
            p -= 1.0;

        const double delay = (1.0 - speed) > 0.0 ? sweep * p : sweep * (1.0 - p);
        const float window = (float) std::pow(std::sin(juce::MathConstants<double>::pi * p), 2.0);
        out += window * readRing(channel, delay + 2.0);
    }

    ring.setSample(channel, writePos, input + 0.35f * lastWet[(size_t) channel]);
    lastWet[(size_t) channel] = out;
    return out;
}

void WetEngine::process(juce::AudioBuffer<float>& buffer)
{
    const int numChannels = juce::jmin(buffer.getNumChannels(), numChannelsPrepared);
    const int numSamples = buffer.getNumSamples();

    if (params.mode != lastMode)
    {
        // Switching modes: start from a clean slate so old echoes don't bleed through.
        ring.clear();
        reverb.reset();
        std::fill(lastWet.begin(), lastWet.end(), 0.0f);
        lastMode = params.mode;
    }

    smoothedMix.setTargetValue(params.mix);

    if (params.mix <= 0.0001f && smoothedMix.getCurrentValue() <= 0.0001f)
    {
        // Keep the delay ring fed so turning the mix up later picks up live material.
        for (int i = 0; i < numSamples; ++i)
        {
            for (int ch = 0; ch < numChannels; ++ch)
                ring.setSample(ch, writePos, buffer.getSample(ch, i));
            writePos = (writePos + 1) % ringLength;
        }
        return;
    }

    wetBuffer.setSize(numChannels, numSamples, false, false, true);
    for (int ch = 0; ch < numChannels; ++ch)
        wetBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);

    if (params.mode == Mode::reverb)
    {
        juce::dsp::Reverb::Parameters rp;
        rp.roomSize = 0.35f + 0.64f * params.time;
        rp.damping = 0.9f - 0.85f * params.modify;
        rp.width = 1.0f;
        rp.wetLevel = 1.0f;
        rp.dryLevel = 0.0f;
        rp.freezeMode = 0.0f;
        reverb.setParameters(rp);

        juce::dsp::AudioBlock<float> block(wetBuffer);
        reverb.process(juce::dsp::ProcessContextReplacing<float>(block));
    }
    else
    {
        // Time knob: 20 ms .. maxDelaySec, exponential.
        const float delaySec = 0.02f * std::pow((float) maxDelaySec / 0.02f, params.time);
        smoothedDelay.setTargetValue(delaySec * (float) sampleRate);

        const double speed = slipSpeedFor(params.modify);
        const float feedback = juce::jlimit(0.0f, 0.95f, params.modify * 0.95f);

        for (int i = 0; i < numSamples; ++i)
        {
            float delaySamples = smoothedDelay.getNextValue();

            // A slip head can reach up to 3 slices back (reverse octave-up), so keep it inside the ring.
            if (params.mode == Mode::slip)
                delaySamples = juce::jmin(delaySamples, (float) (ringLength - 8) / 3.0f);

            for (int ch = 0; ch < numChannels; ++ch)
            {
                const float in = wetBuffer.getSample(ch, i);
                const float out = params.mode == Mode::delay
                                      ? processDelaySample(ch, in, delaySamples, feedback)
                                      : processSlipSample(ch, in, delaySamples, speed);
                wetBuffer.setSample(ch, i, out);
            }

            writePos = (writePos + 1) % ringLength;

            if (params.mode == Mode::slip)
            {
                slipPhase += 1.0 / juce::jmax(1.0f, delaySamples);
                if (slipPhase >= 1.0)
                    slipPhase -= 1.0;
            }
        }
    }

    // Equal-power dry/wet mix.
    for (int i = 0; i < numSamples; ++i)
    {
        const float mix = smoothedMix.getNextValue();
        const float dryGain = std::cos(mix * juce::MathConstants<float>::halfPi);
        const float wetGain = std::sin(mix * juce::MathConstants<float>::halfPi);

        for (int ch = 0; ch < numChannels; ++ch)
            buffer.setSample(ch, i, buffer.getSample(ch, i) * dryGain + wetBuffer.getSample(ch, i) * wetGain);
    }
}
