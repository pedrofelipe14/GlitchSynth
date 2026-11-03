#include "SynthVoice.h"

namespace
{
    // Per-voice output gain, leaves headroom for chords across all voices.
    constexpr float voiceGain = 0.3f;

    /** PolyBLEP residual: smooths the hard edges of saw/square so they don't alias (sound harsh). */
    float polyBlep(float t, float dt)
    {
        if (t < dt)
        {
            t /= dt;
            return t + t - t * t - 1.0f;
        }
        if (t > 1.0f - dt)
        {
            t = (t - 1.0f) / dt;
            return t * t + t + t + 1.0f;
        }
        return 0.0f;
    }
}

float naiveWaveformSample(OscWaveform waveform, float phase)
{
    switch (waveform)
    {
        case OscWaveform::sine:     return std::sin(phase * juce::MathConstants<float>::twoPi);
        case OscWaveform::saw:      return 2.0f * phase - 1.0f;
        case OscWaveform::square:   return phase < 0.5f ? 1.0f : -1.0f;
        case OscWaveform::triangle: return 4.0f * std::abs(phase - 0.5f) - 1.0f;
        case OscWaveform::custom:   break;
    }

    return 0.0f;
}

GlitchSynthVoice::GlitchSynthVoice()
{
    ampAdsr.setParameters({ 0.01f, 0.1f, 0.8f, 0.2f });
}

bool GlitchSynthVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<GlitchSynthSound*>(sound) != nullptr;
}

void GlitchSynthVoice::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    ampAdsr.setSampleRate(sampleRate);

    // ~10 Hz cutoff: inaudible, but removes any constant offset.
    dcBlockCoeff = 1.0f - (float) (juce::MathConstants<double>::twoPi * 10.0 / sampleRate);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels = 1;
    filter.prepare(spec);
    filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
}

void GlitchSynthVoice::startNote(int midiNoteNumber, float velocity,
                                  juce::SynthesiserSound*, int currentPitchWheelPosition)
{
    level = velocity;
    currentNote = midiNoteNumber;
    pitchWheelPosition = currentPitchWheelPosition;
    currentBend = (float) (pitchWheelPosition - 8192) / 8192.0f * pitchBendRange; // start on pitch, no glide-in

    for (auto& osc : oscStates)
        osc.phase = 0.0;

    // If this voice was cut off mid-sound (stolen), fade out what it was playing.
    declickValue = lastOutput;
    dcBlockLastIn = 0.0f;
    dcBlockLastOut = 0.0f;

    filter.reset();
    ampAdsr.reset();
    ampAdsr.noteOn();
}

void GlitchSynthVoice::stopNote(float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff)
    {
        ampAdsr.noteOff();
    }
    else
    {
        clearCurrentNote();
        ampAdsr.reset();
    }
}

void GlitchSynthVoice::pitchWheelMoved(int newPitchWheelValue)
{
    pitchWheelPosition = newPitchWheelValue;
}
void GlitchSynthVoice::controllerMoved(int, int) {}

float GlitchSynthVoice::renderOscSample(int index)
{
    const auto& settings = oscSettings[(size_t) index];
    auto& state = oscStates[(size_t) index];

    float sample = 0.0f;
    const float p = (float) state.phase;
    const float dt = (float) state.phaseIncrement;

    switch (settings.waveform)
    {
        case OscWaveform::saw:
            sample = 2.0f * p - 1.0f - polyBlep(p, dt);
            break;

        case OscWaveform::square:
        {
            float p2 = p + 0.5f;
            if (p2 >= 1.0f)
                p2 -= 1.0f;
            sample = (p < 0.5f ? 1.0f : -1.0f) + polyBlep(p, dt) - polyBlep(p2, dt);
            break;
        }

        case OscWaveform::custom:
            sample = settings.customWave != nullptr ? settings.customWave->sampleAt(p) : 0.0f;
            break;

        case OscWaveform::sine:
        case OscWaveform::triangle:
            sample = naiveWaveformSample(settings.waveform, p);
            break;
    }

    state.phase += state.phaseIncrement;
    if (state.phase >= 1.0)
        state.phase -= 1.0;

    return sample;
}

void GlitchSynthVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                        int startSample, int numSamples)
{
    filter.setResonance(filterResonance);

    const double noteHz = juce::MidiMessage::getMidiNoteInHertz(currentNote);
    std::array<double, numOscillators> baseIncrements;
    for (int o = 0; o < numOscillators; ++o)
        baseIncrements[(size_t) o] = noteHz * std::pow(2.0, oscSettings[(size_t) o].pitchOffsetSemitones / 12.0)
                                   / currentSampleRate;

    // Glide toward the wheel's target over a few ms, so the steps between pitch-bend
    // messages (FL slides are a stream of them) come out as one smooth sweep.
    const float targetBend = (float) (pitchWheelPosition - 8192) / 8192.0f * pitchBendRange;
    const float bendGlideCoeff = (float) std::exp(-1.0 / (0.005 * currentSampleRate));

    const float declickCoeff = (float) std::exp(-1.0 / (0.004 * currentSampleRate));

    for (int i = 0; i < numSamples; ++i)
    {
        if (!ampAdsr.isActive())
        {
            lastOutput = 0.0f;
            declickValue = 0.0f;
            clearCurrentNote();
            break;
        }

        // Same envelope drives amplitude and filter cutoff, so notes "open up" on attack.
        const float envValue = ampAdsr.getNextSample();
        const float modulatedCutoff = juce::jlimit(
            20.0f, 20000.0f,
            filterCutoffHz * std::pow(2.0f, (filterEnvAmount * envValue) / 12.0f));
        filter.setCutoffFrequency(modulatedCutoff);

        currentBend = targetBend + bendGlideCoeff * (currentBend - targetBend);
        const double bendRatio = std::exp2((double) currentBend / 12.0);

        float osc = 0.0f;
        for (int o = 0; o < numOscillators; ++o)
        {
            oscStates[(size_t) o].phaseIncrement = juce::jmin(baseIncrements[(size_t) o] * bendRatio, 0.49);

            const float oscLevel = oscSettings[(size_t) o].level;
            const float sample = renderOscSample(o); // always advance, so phases stay in step
            osc += sample * oscLevel;
        }

        const float dcBlocked = osc - dcBlockLastIn + dcBlockCoeff * dcBlockLastOut;
        dcBlockLastIn = osc;
        dcBlockLastOut = dcBlocked;

        const float filtered = filter.processSample(0, dcBlocked);
        declickValue *= declickCoeff;
        const float out = filtered * level * envValue * voiceGain + declickValue;
        lastOutput = out;

        for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
            outputBuffer.addSample(ch, startSample + i, out);
    }
}
