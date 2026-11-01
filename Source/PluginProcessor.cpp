#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParamIDs.h"

static constexpr int numVoices = 8;

GlitchSynthAudioProcessor::GlitchSynthAudioProcessor()
    : AudioProcessor(BusesProperties()
                          .withOutput("Output", juce::AudioChannelSet::stereo(), true)
                          .withInput("Sidechain", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    synth.clearVoices();
    for (int i = 0; i < numVoices; ++i)
        synth.addVoice(new GlitchSynthVoice());

    synth.clearSounds();
    synth.addSound(new GlitchSynthSound());
}

GlitchSynthAudioProcessor::~GlitchSynthAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout GlitchSynthAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::oscWaveform, "Forma de Onda",
        juce::StringArray { "Seno", "Sierra", "Cuadrada", "Triangular" }, 1));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::ampAttack, "Ataque",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.01f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::ampDecay, "Decaimiento",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.1f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::ampSustain, "Sustain",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.8f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::ampRelease, "Release",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.2f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::filterCutoff, "Corte del Filtro",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f), 4000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::filterResonance, "Resonancia",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.4f), 0.7f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::filterEnvAmount, "Env. del Filtro",
        juce::NormalisableRange<float>(-48.0f, 48.0f, 0.1f), 24.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::masterGainDb, "Volumen General",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::glitchMix, "Mezcla Glitch",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::glitchGrainSizeMs, "Tamano de Grano",
        juce::NormalisableRange<float>(20.0f, 1000.0f, 1.0f, 0.4f), 250.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::glitchPitchRatio, "Tono del Loop",
        juce::NormalisableRange<float>(0.25f, 4.0f, 0.001f, 0.4f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::glitchHoldAmount, "Congelar Loop",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::glitchDegrade, "Degradacion",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::glitchChaos, "Caos",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::glitchReverseAmount, "Reversa",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.1f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::glitchToneTilt, "Tono (Tilt)",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        ParamIDs::selfGlitchEnabled, "Auto-Glitch", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::sidechainLevelDb, "Nivel Sidechain",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f), 0.0f));

    return { params.begin(), params.end() };
}

void GlitchSynthAudioProcessor::updateVoiceParametersFromState()
{
    const auto waveformIndex = (int) apvts.getRawParameterValue(ParamIDs::oscWaveform)->load();

    juce::ADSR::Parameters ampParams;
    ampParams.attack  = apvts.getRawParameterValue(ParamIDs::ampAttack)->load();
    ampParams.decay   = apvts.getRawParameterValue(ParamIDs::ampDecay)->load();
    ampParams.sustain = apvts.getRawParameterValue(ParamIDs::ampSustain)->load();
    ampParams.release = apvts.getRawParameterValue(ParamIDs::ampRelease)->load();

    const float cutoff     = apvts.getRawParameterValue(ParamIDs::filterCutoff)->load();
    const float resonance  = apvts.getRawParameterValue(ParamIDs::filterResonance)->load();
    const float envAmount  = apvts.getRawParameterValue(ParamIDs::filterEnvAmount)->load();

    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<GlitchSynthVoice*>(synth.getVoice(i)))
        {
            voice->setWaveform((OscWaveform) waveformIndex);
            voice->setAmpAdsrParams(ampParams);
            voice->setFilterParams(cutoff, resonance, envAmount);
        }
    }

    GlitchEngine::Params glitchParams;
    glitchParams.mix           = apvts.getRawParameterValue(ParamIDs::glitchMix)->load();
    glitchParams.grainSizeMs   = apvts.getRawParameterValue(ParamIDs::glitchGrainSizeMs)->load();
    glitchParams.pitchRatio    = apvts.getRawParameterValue(ParamIDs::glitchPitchRatio)->load();
    glitchParams.holdAmount    = apvts.getRawParameterValue(ParamIDs::glitchHoldAmount)->load();
    glitchParams.degrade       = apvts.getRawParameterValue(ParamIDs::glitchDegrade)->load();
    glitchParams.chaos         = apvts.getRawParameterValue(ParamIDs::glitchChaos)->load();
    glitchParams.reverseAmount = apvts.getRawParameterValue(ParamIDs::glitchReverseAmount)->load();
    glitchParams.toneTilt      = apvts.getRawParameterValue(ParamIDs::glitchToneTilt)->load();

    sidechainGlitch.setParams(glitchParams);
    selfGlitch.setParams(glitchParams);
}

void GlitchSynthAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate(sampleRate);

    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* voice = dynamic_cast<GlitchSynthVoice*>(synth.getVoice(i)))
            voice->prepare(sampleRate, samplesPerBlock);

    const int numChannels = juce::jmax(1, getMainBusNumOutputChannels());
    sidechainGlitch.prepare(sampleRate, numChannels, samplesPerBlock);
    selfGlitch.prepare(sampleRate, numChannels, samplesPerBlock);

    sidechainWorkBuffer.setSize(numChannels, samplesPerBlock);
}

void GlitchSynthAudioProcessor::releaseResources() {}

bool GlitchSynthAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto mainOut = layouts.getMainOutputChannelSet();
    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.inputBuses.size() > 0)
    {
        const auto sideIn = layouts.getChannelSet(true, 0);
        if (!sideIn.isDisabled() && sideIn != mainOut)
            return false;
    }

    return true;
}

void GlitchSynthAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    updateVoiceParametersFromState();

    const int numSamples = buffer.getNumSamples();

    // Merge on-screen keyboard clicks (from the editor) with any incoming MIDI,
    // so the plugin can be played standalone without an external MIDI controller.
    keyboardState.processNextMidiBuffer(midiMessages, 0, numSamples, true);

    auto mainOut = getBusBuffer(buffer, false, 0);
    mainOut.clear();
    synth.renderNextBlock(mainOut, midiMessages, 0, numSamples);

    if (apvts.getRawParameterValue(ParamIDs::selfGlitchEnabled)->load() > 0.5f)
        selfGlitch.process(mainOut);

    if (getBusCount(true) > 0)
    {
        auto sideIn = getBusBuffer(buffer, true, 0);

        if (sideIn.getNumChannels() > 0 && getBus(true, 0)->isEnabled())
        {
            sidechainWorkBuffer.setSize(sideIn.getNumChannels(), numSamples, false, false, true);
            sidechainWorkBuffer.makeCopyOf(sideIn, true);

            sidechainGlitch.process(sidechainWorkBuffer);

            const float sidechainGain = juce::Decibels::decibelsToGain(
                apvts.getRawParameterValue(ParamIDs::sidechainLevelDb)->load());

            for (int ch = 0; ch < mainOut.getNumChannels(); ++ch)
            {
                const int srcCh = juce::jmin(ch, sidechainWorkBuffer.getNumChannels() - 1);
                mainOut.addFrom(ch, 0, sidechainWorkBuffer, srcCh, 0, numSamples, sidechainGain);
            }
        }
    }

    const float masterGain = juce::Decibels::decibelsToGain(
        apvts.getRawParameterValue(ParamIDs::masterGainDb)->load());
    mainOut.applyGain(masterGain);
}

juce::AudioProcessorEditor* GlitchSynthAudioProcessor::createEditor()
{
    return new GlitchSynthAudioProcessorEditor(*this);
}

void GlitchSynthAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); true)
    {
        std::unique_ptr<juce::XmlElement> xml(state.createXml());
        copyXmlToBinary(*xml, destData);
    }
}

void GlitchSynthAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GlitchSynthAudioProcessor();
}
