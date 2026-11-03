#include "GlitchEngine.h"

namespace
{
    // Consonant intervals a grain may jump to: unison, fifths and octaves always
    // sound "in key" relative to whatever is being played.
    constexpr std::array<int, 7> jumpIntervals { -12, -5, 0, 7, 12, 19, 24 };

    constexpr float gateThreshold = 0.0005f;    // ~ -66 dBFS
    constexpr double envelopeReleaseSec = 0.08;
    constexpr double gateOpenSec = 0.005;
    constexpr double gateCloseSec = 0.25;
    constexpr double onsetCaptureDelaySec = 0.12;
}

void GlitchEngine::prepare(double newSampleRate, int numChannels, int /*maximumBlockSize*/)
{
    sampleRate = newSampleRate;
    numChannelsPrepared = numChannels;

    liveRingLengthSamples = juce::jmax(1, (int) std::round(sampleRate * 2.5));
    liveRing.setSize(numChannels, liveRingLengthSamples);
    liveRing.clear();
    liveWritePos = 0;

    // Pre-allocate both grain buffers at the maximum size so the audio thread never allocates.
    for (auto& g : grains)
    {
        g.setSize(numChannels, liveRingLengthSamples);
        g.clear();
    }

    heldSample.assign((size_t) numChannels, 0.0f);
    holdCounter.assign((size_t) numChannels, 0);
    tiltLpState.assign((size_t) numChannels, 0.0f);

    reset();
}

void GlitchEngine::reset()
{
    for (auto& h : heads)
        h = Head {};

    grainLengths = { 1, 1 };
    mainHead = 0;
    repeatCount = 0;
    currentSemitones = 0;

    inputEnvelope = 0.0f;
    gateGain = 0.0f;
    inputActive = false;
    samplesSinceOnset = 0;
    pendingCaptureCountdown = -1;

    std::fill(holdCounter.begin(), holdCounter.end(), 0);
    std::fill(heldSample.begin(), heldSample.end(), 0.0f);
    std::fill(tiltLpState.begin(), tiltLpState.end(), 0.0f);

    wowCurrent = 0.0f;
    wowTarget = 0.0f;
    wowCounter = 0;
}

int GlitchEngine::fadeLengthFor(const Head& head) const
{
    const int len = grainLengths[(size_t) head.bufferIndex];
    const double maxFade = sampleRate * 0.012;
    const double fitFade = (double) len / (3.0 * juce::jmax(0.01, head.rate));
    return juce::jmax(8, (int) std::min(maxFade, fitFade));
}

void GlitchEngine::startGrain(bool captureFresh, int captureLength)
{
    auto& oldHead = heads[(size_t) mainHead];
    const int newIndex = 1 - mainHead;
    auto& newHead = heads[(size_t) newIndex];

    int bufferIndex = 0;
    bool reversed = false;

    if (captureFresh || !oldHead.active)
    {
        bufferIndex = oldHead.active ? 1 - oldHead.bufferIndex : 0;

        const int minLen = juce::jmax(1, (int) std::round(sampleRate * 0.01));
        const int len = juce::jlimit(minLen, liveRingLengthSamples, captureLength);
        auto& grain = grains[(size_t) bufferIndex];

        for (int ch = 0; ch < numChannelsPrepared; ++ch)
        {
            for (int i = 0; i < len; ++i)
            {
                int srcIndex = (liveWritePos - len + i) % liveRingLengthSamples;
                if (srcIndex < 0)
                    srcIndex += liveRingLengthSamples;

                grain.setSample(ch, i, liveRing.getSample(ch, srcIndex));
            }
        }

        grainLengths[(size_t) bufferIndex] = len;
        reversed = random.nextFloat() < params.reverseAmount;
        repeatCount = 0;

        currentSemitones = params.intervalSemitones;
        if (random.nextFloat() < params.jumpChance)
            currentSemitones = jumpIntervals[(size_t) random.nextInt((int) jumpIntervals.size())];
    }
    else
    {
        bufferIndex = oldHead.bufferIndex;
        reversed = oldHead.reversed;

        if (params.chaos > 0.0f && random.nextFloat() < params.chaos * 0.5f)
            reversed = !reversed;

        ++repeatCount;
    }

    const int len = grainLengths[(size_t) bufferIndex];

    newHead.active = true;
    newHead.bufferIndex = bufferIndex;
    newHead.reversed = reversed;
    newHead.rate = std::pow(2.0, currentSemitones / 12.0);
    newHead.pos = reversed ? (double) (len - 1) : 0.0;
    newHead.gain = 0.0f;
    newHead.fadeDirection = 1;
    newHead.fadeStep = 1.0f / (float) fadeLengthFor(newHead);

    if (oldHead.active)
    {
        oldHead.fadeDirection = -1;
        oldHead.fadeStep = 1.0f / (float) fadeLengthFor(oldHead);
    }

    mainHead = newIndex;
}

float GlitchEngine::readHead(const Head& head, int channel) const
{
    const auto& grain = grains[(size_t) head.bufferIndex];
    const int len = grainLengths[(size_t) head.bufferIndex];

    const int i0 = juce::jlimit(0, len - 1, (int) std::floor(head.pos));
    const int i1 = juce::jmin(i0 + 1, len - 1);
    const float frac = (float) (head.pos - std::floor(head.pos));

    const float s0 = grain.getSample(channel, i0);
    const float s1 = grain.getSample(channel, i1);
    return s0 + frac * (s1 - s0);
}

float GlitchEngine::applyDegrade(int channel, float sample)
{
    const float repeatFactor = juce::jlimit(0.0f, 1.0f, (float) repeatCount / 8.0f);
    const float intensity = juce::jlimit(0.0f, 1.0f,
        params.degrade * 0.6f + params.degrade * repeatFactor * 0.4f);

    if (intensity <= 0.0f)
        return sample;

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

    crushed += (random.nextFloat() * 2.0f - 1.0f) * intensity * 0.03f * std::abs(held);

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
        // Kept within ~±20 cents so the wobble sounds like tape, not out of tune.
        wowTarget = (random.nextFloat() * 2.0f - 1.0f) * 0.012f;
    }

    wowCurrent += 0.0005f * (wowTarget - wowCurrent);
    return wowCurrent * juce::jlimit(0.0f, 1.0f, params.chaos);
}

void GlitchEngine::process(juce::AudioBuffer<float>& buffer)
{
    const int numChannels = juce::jmin(buffer.getNumChannels(), numChannelsPrepared);
    const int numSamples = buffer.getNumSamples();

    const int grainLenSamples = juce::jlimit(
        1, liveRingLengthSamples,
        (int) std::round(sampleRate * (double) juce::jmax(10.0f, params.grainSizeMs) / 1000.0));

    const float envRelease = (float) std::exp(-1.0 / (envelopeReleaseSec * sampleRate));
    const float gateUpStep = (float) (1.0 / (gateOpenSec * sampleRate));
    const float gateDownStep = (float) (1.0 / (gateCloseSec * sampleRate));

    for (int i = 0; i < numSamples; ++i)
    {
        // Record input and follow its envelope.
        float peak = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float s = buffer.getSample(ch, i);
            liveRing.setSample(ch, liveWritePos, s);
            peak = juce::jmax(peak, std::abs(s));
        }
        liveWritePos = (liveWritePos + 1) % liveRingLengthSamples;

        inputEnvelope = juce::jmax(peak, inputEnvelope * envRelease);
        const bool nowActive = inputEnvelope > gateThreshold;

        if (nowActive && !inputActive)
        {
            // New note: wait a moment so the grain we grab is the new note, not the last one.
            samplesSinceOnset = 0;
            pendingCaptureCountdown = juce::jmin(grainLenSamples, (int) (onsetCaptureDelaySec * sampleRate));
        }

        inputActive = nowActive;
        if (inputActive)
            ++samplesSinceOnset;

        if (pendingCaptureCountdown > 0 && --pendingCaptureCountdown == 0)
        {
            startGrain(true, juce::jmin(grainLenSamples, samplesSinceOnset));
            pendingCaptureCountdown = -1;
        }

        // Loop boundary: crossfade into the next grain (fresh capture or a repeat).
        {
            auto& h = heads[(size_t) mainHead];
            if (h.active && h.fadeDirection >= 0)
            {
                const int len = grainLengths[(size_t) h.bufferIndex];
                const double remaining = h.reversed ? h.pos : (double) (len - 1) - h.pos;

                if (remaining <= (double) fadeLengthFor(h) * h.rate)
                {
                    const bool fresh = inputActive && random.nextFloat() >= params.holdAmount;
                    startGrain(fresh, juce::jmin(grainLenSamples, juce::jmax(1, samplesSinceOnset)));
                }
            }
        }

        const bool anyHeadActive = heads[0].active || heads[1].active;
        const bool gateOpen = params.keepLooping ? anyHeadActive
                                                 : (inputActive && pendingCaptureCountdown < 0);
        gateGain = gateOpen ? juce::jmin(1.0f, gateGain + gateUpStep)
                            : juce::jmax(0.0f, gateGain - gateDownStep);

        const float effectiveMix = params.mix * gateGain;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float wet = 0.0f;
            for (const auto& h : heads)
                if (h.active)
                    wet += h.gain * readHead(h, ch);

            wet = applyDegrade(ch, wet);
            wet = applyToneTilt(ch, wet);

            const float dry = buffer.getSample(ch, i);
            buffer.setSample(ch, i, dry + effectiveMix * (wet - dry));
        }

        // Advance playheads and their crossfade gains.
        const double wow = 1.0 + (double) nextWowOffset();

        for (int hi = 0; hi < (int) heads.size(); ++hi)
        {
            auto& h = heads[(size_t) hi];
            if (!h.active)
                continue;

            const int len = grainLengths[(size_t) h.bufferIndex];
            h.pos += (h.reversed ? -1.0 : 1.0) * h.rate * wow;

            if (h.pos < 0.0 || h.pos > (double) (len - 1))
            {
                if (hi != mainHead || h.fadeDirection < 0)
                {
                    h.active = false;
                    continue;
                }

                h.pos = h.reversed ? (double) (len - 1) : 0.0;
            }

            if (h.fadeDirection > 0)
            {
                h.gain += h.fadeStep;
                if (h.gain >= 1.0f) { h.gain = 1.0f; h.fadeDirection = 0; }
            }
            else if (h.fadeDirection < 0)
            {
                h.gain -= h.fadeStep;
                if (h.gain <= 0.0f) { h.gain = 0.0f; h.active = false; }
            }
        }
    }
}
