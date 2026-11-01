#include "SynthVoice.h"

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

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels = 1;
    filter.prepare(spec);
    filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
}

void GlitchSynthVoice::startNote(int midiNoteNumber, float velocity,
                                  juce::SynthesiserSound*, int /*currentPitchWheelPosition*/)
{
    level = velocity;
    phase = 0.0;

    const double freq = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
    phaseIncrement = freq / currentSampleRate;

    filter.reset();
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

void GlitchSynthVoice::pitchWheelMoved(int) {}
void GlitchSynthVoice::controllerMoved(int, int) {}

float GlitchSynthVoice::renderOscSample()
{
    float sample = 0.0f;
    const float p = (float) phase;

    switch (waveform)
    {
        case OscWaveform::sine:
            sample = std::sin(p * juce::MathConstants<float>::twoPi);
            break;

        case OscWaveform::saw:
            sample = 2.0f * p - 1.0f;
            break;

        case OscWaveform::square:
            sample = p < 0.5f ? 1.0f : -1.0f;
            break;

        case OscWaveform::triangle:
            sample = 4.0f * std::abs(p - 0.5f) - 1.0f;
            break;
    }

    phase += phaseIncrement;
    if (phase >= 1.0)
        phase -= 1.0;

    return sample;
}

void GlitchSynthVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                        int startSample, int numSamples)
{
    filter.setResonance(filterResonance);

    for (int i = 0; i < numSamples; ++i)
    {
        if (!ampAdsr.isActive())
        {
            clearCurrentNote();
            break;
        }

        // Same envelope drives amplitude and filter cutoff, so notes "open up" on attack.
        const float envValue = ampAdsr.getNextSample();
        const float modulatedCutoff = juce::jlimit(
            20.0f, 20000.0f,
            filterCutoffHz * std::pow(2.0f, (filterEnvAmount * envValue) / 12.0f));
        filter.setCutoffFrequency(modulatedCutoff);

        const float osc = renderOscSample();
        const float filtered = filter.processSample(0, osc);
        const float out = filtered * level * envValue;

        for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
            outputBuffer.addSample(ch, startSample + i, out);
    }
}
