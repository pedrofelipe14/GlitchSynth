#include "GlitchEngine.h"

void GlitchEngine::prepare(double newSampleRate, int numChannels, int /*maximumBlockSize*/)
{
    sampleRate = newSampleRate;

    liveRingLengthSamples = juce::jmax(1, (int) std::round(sampleRate * 2.5));
    liveRing.setSize(numChannels, liveRingLengthSamples);
    liveRing.clear();
    liveWritePos = 0;

    grainSnapshot.setSize(numChannels, 0);
    grainLengthSamples = 1;

    heldSample.assign((size_t) numChannels, 0.0f);
    holdCounter.assign((size_t) numChannels, 0);
    tiltLpState.assign((size_t) numChannels, 0.0f);

    reset();
}

void GlitchEngine::reset()
{
    playbackPos = 0.0;
    playingReversed = false;
    repeatCount = 0;

    std::fill(holdCounter.begin(), holdCounter.end(), 0);
    std::fill(heldSample.begin(), heldSample.end(), 0.0f);
    std::fill(tiltLpState.begin(), tiltLpState.end(), 0.0f);

    wowCurrent = 0.0f;
    wowTarget = 0.0f;
    wowCounter = 0;
}

void GlitchEngine::captureNewGrain(int numChannels)
{
    const int minLen = juce::jmax(1, (int) std::round(sampleRate * 0.005));
    const int newLen = juce::jlimit(
        minLen, liveRingLengthSamples,
        (int) std::round(sampleRate * (double) juce::jmax(5.0f, params.grainSizeMs) / 1000.0));

    grainLengthSamples = newLen;

    if (grainSnapshot.getNumChannels() != numChannels || grainSnapshot.getNumSamples() != grainLengthSamples)
        grainSnapshot.setSize(numChannels, grainLengthSamples, false, false, true);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        for (int i = 0; i < grainLengthSamples; ++i)
        {
            int srcIndex = (liveWritePos - grainLengthSamples + i) % liveRingLengthSamples;
            if (srcIndex < 0)
                srcIndex += liveRingLengthSamples;

            grainSnapshot.setSample(ch, i, liveRing.getSample(ch, srcIndex));
        }
    }

    playingReversed = random.nextFloat() < params.reverseAmount;
    playbackPos = playingReversed ? (double) (grainLengthSamples - 1) : 0.0;
    repeatCount = 0;
}

void GlitchEngine::onGrainCycleComplete(int numChannels)
{
    if (params.chaos > 0.0f && random.nextFloat() < params.chaos * 0.5f)
        playingReversed = !playingReversed;

    if (random.nextFloat() > params.holdAmount)
    {
        captureNewGrain(numChannels);
    }
    else
    {
        ++repeatCount;
        playbackPos = playingReversed ? (double) (grainLengthSamples - 1) : 0.0;
    }
}

float GlitchEngine::applyDegrade(int channel, float sample)
{
    const float repeatFactor = juce::jlimit(0.0f, 1.0f, (float) repeatCount / 8.0f);
    const float intensity = juce::jlimit(0.0f, 1.0f,
        params.degrade * 0.6f + params.degrade * repeatFactor * 0.4f);

    const int divider = 1 + (int) std::round(intensity * 24.0f);
    auto& counter = holdCounter[(size_t) channel];
    auto& held = heldSample[(size_t) channel];

    if (counter <= 0)
    {
        held = sample;
        counter = divider;
    }
    --counter;

    const float bits = juce::jmap(intensity, 0.0f, 1.0f, 16.0f, 3.0f);
    const float levels = std::pow(2.0f, bits);
    float crushed = std::round(held * levels) / levels;

    if (intensity > 0.0f)
        crushed += (random.nextFloat() * 2.0f - 1.0f) * intensity * 0.03f;

    return crushed;
}

float GlitchEngine::applyToneTilt(int channel, float sample)
{
    constexpr float lpCoeff = 0.15f;
    auto& lp = tiltLpState[(size_t) channel];
    lp += lpCoeff * (sample - lp);
    const float hp = sample - lp;

    const float tilt = juce::jlimit(0.0f, 1.0f, params.toneTilt);
    return lp + tilt * 2.0f * hp;
}

float GlitchEngine::nextWowOffset()
{
    if (--wowCounter <= 0)
    {
        wowCounter = 1 + random.nextInt(juce::jmax(1, (int) (sampleRate * 0.15)));
        wowTarget = (random.nextFloat() * 2.0f - 1.0f) * 0.06f;
    }

    wowCurrent += 0.01f * (wowTarget - wowCurrent);
    return wowCurrent * juce::jlimit(0.0f, 1.0f, params.chaos);
}

void GlitchEngine::process(juce::AudioBuffer<float>& buffer)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    if (grainSnapshot.getNumSamples() == 0 || grainSnapshot.getNumChannels() != numChannels)
        captureNewGrain(numChannels);

    const double basePitch = juce::jlimit(0.1, 8.0, (double) params.pitchRatio);

    for (int i = 0; i < numSamples; ++i)
    {
        for (int ch = 0; ch < numChannels; ++ch)
            liveRing.setSample(ch, liveWritePos, buffer.getSample(ch, i));
        liveWritePos = (liveWritePos + 1) % liveRingLengthSamples;

        const double pitchThisSample = juce::jlimit(0.05, 8.0, basePitch + (double) nextWowOffset());
        const int len = juce::jmax(1, grainLengthSamples);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float drySample = buffer.getSample(ch, i);

            const int i0 = (int) std::floor(playbackPos);
            const int ci0 = juce::jlimit(0, grainSnapshot.getNumSamples() - 1, i0 % len);
            const int ci1 = juce::jlimit(0, grainSnapshot.getNumSamples() - 1, (i0 + 1) % len);
            const float frac = (float) (playbackPos - (double) i0);

            const float s0 = grainSnapshot.getSample(ch, ci0);
            const float s1 = grainSnapshot.getSample(ch, ci1);
            float wetSample = s0 + frac * (s1 - s0);

            wetSample = applyDegrade(ch, wetSample);
            wetSample = applyToneTilt(ch, wetSample);

            buffer.setSample(ch, i, drySample + params.mix * (wetSample - drySample));
        }

        playbackPos += playingReversed ? -pitchThisSample : pitchThisSample;

        if (playingReversed)
        {
            if (playbackPos < 0.0)
            {
                playbackPos += grainLengthSamples;
                onGrainCycleComplete(numChannels);
            }
        }
        else if (playbackPos >= grainLengthSamples)
        {
            playbackPos -= grainLengthSamples;
            onGrainCycleComplete(numChannels);
        }
    }
}
