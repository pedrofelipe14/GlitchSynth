#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <array>

/** Granular micro-looper / degrader.
    Continuously records live audio, periodically freezes a grain and loops it,
    pitch-shifts the loop by musical intervals only, progressively degrades it
    the longer it repeats, and lets a single "chaos" control introduce
    generative randomness (tape wow/flutter, random reverse flips).

    Every loop boundary is crossfaded between two playheads so nothing clicks,
    and the loop follows the input: when the input goes silent the loop fades
    out (unless keepLooping is on), and a new note immediately grabs a fresh
    grain of itself instead of replaying the previous note. */
class GlitchEngine
{
public:
    void prepare(double sampleRate, int numChannels, int maximumBlockSize);
    void reset();

    struct Params
    {
        float mix = 1.0f;            // 0 = dry, 1 = fully glitched
        float grainSizeMs = 250.0f;  // length of the captured loop/grain
        int intervalSemitones = 0;   // base transposition of the loop, in semitones
        float jumpChance = 0.0f;     // chance a new grain jumps to a random consonant interval
        float holdAmount = 0.0f;     // 0 = keep grabbing fresh grains, 1 = repeat the same grain
        float degrade = 0.0f;        // bit/rate crush + grit, intensified the longer a grain repeats
        float chaos = 0.0f;          // generative randomness: wow/flutter, random reverse flips
        float reverseAmount = 0.0f;  // base chance a freshly captured grain plays backwards
        float toneTilt = 0.5f;       // 0 = dark/lowpassed, 1 = bright/highpass-emphasised
        bool keepLooping = false;    // true = loop keeps sounding after the input goes silent
    };

    void setParams(const Params& newParams) { params = newParams; }

    /** Processes buffer in place. */
    void process(juce::AudioBuffer<float>& buffer);

    /** Intervals (in semitones) offered by the loop-interval parameter. */
    static constexpr std::array<int, 10> intervalChoices { -24, -12, -7, -5, 0, 5, 7, 12, 19, 24 };

private:
    struct Head
    {
        bool active = false;
        int bufferIndex = 0;
        double pos = 0.0;
        double rate = 1.0;
        bool reversed = false;
        float gain = 0.0f;
        float fadeStep = 0.01f;
        int fadeDirection = 0; // +1 fading in, -1 fading out, 0 steady
    };

    void startGrain(bool captureFresh, int captureLength);
    float readHead(const Head& head, int channel) const;
    int fadeLengthFor(const Head& head) const;
    float applyDegrade(int channel, float sample);
    float applyToneTilt(int channel, float sample);
    float nextWowOffset();

    double sampleRate = 44100.0;
    int numChannelsPrepared = 0;
    Params params;

    // Continuous live-input recording ring; source material for fresh grain captures.
    juce::AudioBuffer<float> liveRing;
    int liveRingLengthSamples = 1;
    int liveWritePos = 0;

    // Two grain buffers: a fresh capture goes into the one the fading-out head isn't using.
    std::array<juce::AudioBuffer<float>, 2> grains;
    std::array<int, 2> grainLengths { 1, 1 };

    std::array<Head, 2> heads;
    int mainHead = 0;
    int repeatCount = 0;
    int currentSemitones = 0;

    // Input-following gate.
    float inputEnvelope = 0.0f;
    float gateGain = 0.0f;
    bool inputActive = false;
    int samplesSinceOnset = 0;
    int pendingCaptureCountdown = -1;

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
