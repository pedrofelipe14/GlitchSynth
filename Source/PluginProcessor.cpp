#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParamIDs.h"
#include "Presets.h"

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

namespace
{
    /** How a parameter's value reads on screen and in the host ("4.50 kHz", "300 ms"...). */
    enum class Unit { plain, percent, seconds, milliseconds, hertz, decibels, semitones, cents, octaves, q };

    juce::String withSign(float value, int decimals)
    {
        return (value > 0.0f ? "+" : "") + juce::String(value, decimals);
    }

    juce::String formatValue(Unit unit, float v)
    {
        switch (unit)
        {
            case Unit::percent:      return juce::String(juce::roundToInt(v * 100.0f)) + " %";
            case Unit::seconds:      return v < 1.0f ? juce::String(juce::roundToInt(v * 1000.0f)) + " ms"
                                                     : juce::String(v, 2) + " s";
            case Unit::milliseconds: return juce::String(juce::roundToInt(v)) + " ms";
            case Unit::hertz:        return v < 10.0f   ? juce::String(v, 2) + " Hz"
                                       : v < 1000.0f ? juce::String(juce::roundToInt(v)) + " Hz"
                                                     : juce::String(v / 1000.0f, 2) + " kHz";
            case Unit::decibels:     return withSign(v, 1) + " dB";
            case Unit::semitones:    return withSign(v, std::abs(v - std::round(v)) < 0.05f ? 0 : 1) + " st";
            case Unit::cents:        return withSign(v, 1) + " ct";
            case Unit::octaves:      return withSign(v, 0) + " oct";
            case Unit::q:            return juce::String(v, 2);
            case Unit::plain:        break;
        }

        return juce::String(v, 2);
    }

    /** Reads typed-in text back, so "4.5 kHz", "300 ms" or "55 %" land where expected. */
    float parseValue(Unit unit, const juce::String& text)
    {
        const float number = text.getFloatValue();
        const auto lower = text.toLowerCase();

        switch (unit)
        {
            case Unit::percent: return number / 100.0f;
            case Unit::seconds: return lower.contains("ms") ? number / 1000.0f : number;
            case Unit::hertz:   return lower.contains("k") ? number * 1000.0f : number;
            default:            return number;
        }
    }

    std::unique_ptr<juce::AudioParameterFloat> makeFloat(const juce::String& id, const juce::String& name,
                                                         juce::NormalisableRange<float> range,
                                                         float defaultValue, Unit unit)
    {
        return std::make_unique<juce::AudioParameterFloat>(
            id, name, range, defaultValue,
            juce::AudioParameterFloatAttributes()
                .withStringFromValueFunction([unit](float v, int) { return formatValue(unit, v); })
                .withValueFromStringFunction([unit](const juce::String& t) { return parseValue(unit, t); }));
    }

    std::unique_ptr<juce::AudioParameterInt> makeInt(const juce::String& id, const juce::String& name,
                                                     int minValue, int maxValue, int defaultValue, Unit unit)
    {
        return std::make_unique<juce::AudioParameterInt>(
            id, name, minValue, maxValue, defaultValue,
            juce::AudioParameterIntAttributes()
                .withStringFromValueFunction([unit](int v, int) { return formatValue(unit, (float) v); })
                .withValueFromStringFunction([](const juce::String& t) { return t.getIntValue(); }));
    }

    juce::NormalisableRange<float> unitRange() { return { 0.0f, 1.0f, 0.001f }; }
}

juce::AudioProcessorValueTreeState::ParameterLayout GlitchSynthAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Osc 2 starts silent and slightly detuned: raise its level for an instant fatter sound.
    for (int i = 0; i < GlitchSynthVoice::numOscillators; ++i)
    {
        const auto& ids = ParamIDs::oscillators[(size_t) i];
        const juce::String prefix = "Osc " + juce::String(i + 1) + " ";
        const bool first = i == 0;

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            ids.waveform, prefix + "Forma",
            juce::StringArray { "Seno", "Sierra", "Cuadrada", "Triangular", "Propia" }, 1));
        params.push_back(makeFloat(ids.level, prefix + "Nivel", unitRange(), first ? 1.0f : 0.0f, Unit::percent));
        params.push_back(makeInt(ids.octave, prefix + "Octava", -3, 3, 0, Unit::octaves));
        params.push_back(makeInt(ids.semitone, prefix + "Semitonos", -12, 12, 0, Unit::semitones));
        params.push_back(makeFloat(ids.fine, prefix + "Fino",
                                   { -50.0f, 50.0f, 0.1f }, first ? 0.0f : 7.0f, Unit::cents));
    }

    const juce::NormalisableRange<float> envTimeRange { 0.001f, 5.0f, 0.001f, 0.3f };
    params.push_back(makeFloat(ParamIDs::ampAttack,  "Ataque",      envTimeRange, 0.01f, Unit::seconds));
    params.push_back(makeFloat(ParamIDs::ampDecay,   "Decaimiento", envTimeRange, 0.1f,  Unit::seconds));
    params.push_back(makeFloat(ParamIDs::ampSustain, "Sustain",     unitRange(),  0.8f,  Unit::percent));
    params.push_back(makeFloat(ParamIDs::ampRelease, "Release",     envTimeRange, 0.2f,  Unit::seconds));

    params.push_back(makeFloat(ParamIDs::filterCutoff, "Corte del Filtro",
                               { 20.0f, 20000.0f, 1.0f, 0.3f }, 4000.0f, Unit::hertz));
    params.push_back(makeFloat(ParamIDs::filterResonance, "Resonancia",
                               { 0.1f, 10.0f, 0.01f, 0.4f }, 0.7f, Unit::q));
    params.push_back(makeFloat(ParamIDs::filterEnvAmount, "Env. del Filtro",
                               { -48.0f, 48.0f, 0.1f }, 24.0f, Unit::semitones));

    // FL Studio's wrapper bends 12 semitones by default (slide notes are sent as
    // pitch bend), so matching that makes slides land on the right note out of the box.
    params.push_back(makeInt(ParamIDs::pitchBendRange, "Rango Pitch Bend", 1, 24, 12, Unit::semitones));

    // Depth starts at zero, so the LFO is silent until you reach for it.
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::lfoShape, "LFO Forma",
        juce::StringArray { "Seno", "Triangular", "Sierra", "Cuadrada", "Azar" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::lfoTarget, "LFO Destino",
        juce::StringArray { "Filtro", "Tono", "Volumen" }, 0));
    params.push_back(makeFloat(ParamIDs::lfoRate, "LFO Velocidad",
                               { 0.05f, 20.0f, 0.01f, 0.3f }, 4.0f, Unit::hertz));
    params.push_back(makeFloat(ParamIDs::lfoDepth, "LFO Profundidad", unitRange(), 0.0f, Unit::percent));

    params.push_back(makeFloat(ParamIDs::masterGainDb, "Volumen General",
                               { -24.0f, 12.0f, 0.1f }, 0.0f, Unit::decibels));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::wetMode, "Modo Wet",
        juce::StringArray { "Reverb", "Delay", "Slip" }, 0));
    params.push_back(makeFloat(ParamIDs::wetMix,    "Mezcla Wet", unitRange(), 0.0f, Unit::percent));
    params.push_back(makeFloat(ParamIDs::wetTime,   "Tiempo",     unitRange(), 0.5f, Unit::percent));
    params.push_back(makeFloat(ParamIDs::wetModify, "Modificar",  unitRange(), 0.5f, Unit::percent));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::routeOrder, "Orden",
        juce::StringArray { "Loop > Wet", "Wet > Loop" }, 0));

    params.push_back(makeFloat(ParamIDs::glitchMix, "Mezcla Glitch", unitRange(), 0.0f, Unit::percent));
    params.push_back(makeFloat(ParamIDs::glitchGrainSizeMs, "Tamano de Grano",
                               { 20.0f, 1000.0f, 1.0f, 0.4f }, 250.0f, Unit::milliseconds));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::loopInterval, "Intervalo del Loop",
        juce::StringArray { "-2 oct", "-1 oct", "-5ta", "-4ta", "Unisono",
                            "+4ta", "+5ta", "+1 oct", "+1 oct +5ta", "+2 oct" }, 4));
    params.push_back(makeFloat(ParamIDs::loopJumps,           "Saltos de Tono", unitRange(), 0.0f, Unit::percent));
    params.push_back(makeFloat(ParamIDs::glitchHoldAmount,    "Congelar Loop",  unitRange(), 0.3f, Unit::percent));
    params.push_back(makeFloat(ParamIDs::glitchDegrade,       "Degradacion",    unitRange(), 0.2f, Unit::percent));
    params.push_back(makeFloat(ParamIDs::glitchChaos,         "Caos",           unitRange(), 0.2f, Unit::percent));
    params.push_back(makeFloat(ParamIDs::glitchReverseAmount, "Reversa",        unitRange(), 0.1f, Unit::percent));
    params.push_back(makeFloat(ParamIDs::glitchToneTilt,      "Tono (Tilt)",    unitRange(), 0.5f, Unit::percent));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        ParamIDs::selfGlitchEnabled, "Auto-Glitch", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        ParamIDs::loopKeep, "Mantener Loop", false));
    params.push_back(makeFloat(ParamIDs::sidechainLevelDb, "Nivel Sidechain",
                               { -24.0f, 12.0f, 0.1f }, 0.0f, Unit::decibels));

    return { params.begin(), params.end() };
}

void GlitchSynthAudioProcessor::updateVoiceParametersFromState()
{
    std::array<GlitchSynthVoice::OscSettings, GlitchSynthVoice::numOscillators> oscSettings;
    for (int o = 0; o < GlitchSynthVoice::numOscillators; ++o)
    {
        const auto& ids = ParamIDs::oscillators[(size_t) o];
        auto& settings = oscSettings[(size_t) o];
        settings.waveform = (OscWaveform) (int) apvts.getRawParameterValue(ids.waveform)->load();
        settings.level = apvts.getRawParameterValue(ids.level)->load();
        settings.pitchOffsetSemitones = 12.0f * apvts.getRawParameterValue(ids.octave)->load()
                                      + apvts.getRawParameterValue(ids.semitone)->load()
                                      + apvts.getRawParameterValue(ids.fine)->load() / 100.0f;
        settings.customWave = &customWaves[(size_t) o];
    }

    juce::ADSR::Parameters ampParams;
    ampParams.attack  = apvts.getRawParameterValue(ParamIDs::ampAttack)->load();
    ampParams.decay   = apvts.getRawParameterValue(ParamIDs::ampDecay)->load();
    ampParams.sustain = apvts.getRawParameterValue(ParamIDs::ampSustain)->load();
    ampParams.release = apvts.getRawParameterValue(ParamIDs::ampRelease)->load();

    const float cutoff     = apvts.getRawParameterValue(ParamIDs::filterCutoff)->load();
    const float resonance  = apvts.getRawParameterValue(ParamIDs::filterResonance)->load();
    const float envAmount  = apvts.getRawParameterValue(ParamIDs::filterEnvAmount)->load();
    const float bendRange  = apvts.getRawParameterValue(ParamIDs::pitchBendRange)->load();

    GlitchSynthVoice::LfoSettings lfo;
    lfo.shape  = (LfoShape) (int) apvts.getRawParameterValue(ParamIDs::lfoShape)->load();
    lfo.target = (LfoTarget) (int) apvts.getRawParameterValue(ParamIDs::lfoTarget)->load();
    lfo.rateHz = apvts.getRawParameterValue(ParamIDs::lfoRate)->load();
    lfo.depth  = apvts.getRawParameterValue(ParamIDs::lfoDepth)->load();

    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<GlitchSynthVoice*>(synth.getVoice(i)))
        {
            for (int o = 0; o < GlitchSynthVoice::numOscillators; ++o)
                voice->setOscillator(o, oscSettings[(size_t) o]);
            voice->setAmpAdsrParams(ampParams);
            voice->setFilterParams(cutoff, resonance, envAmount);
            voice->setPitchBendRange(bendRange);
            voice->setLfo(lfo);
        }
    }

    GlitchEngine::Params glitchParams;
    glitchParams.mix           = apvts.getRawParameterValue(ParamIDs::glitchMix)->load();
    glitchParams.grainSizeMs   = apvts.getRawParameterValue(ParamIDs::glitchGrainSizeMs)->load();
    const int intervalIndex = juce::jlimit(0, (int) GlitchEngine::intervalChoices.size() - 1,
        (int) apvts.getRawParameterValue(ParamIDs::loopInterval)->load());
    glitchParams.intervalSemitones = GlitchEngine::intervalChoices[(size_t) intervalIndex];
    glitchParams.jumpChance    = apvts.getRawParameterValue(ParamIDs::loopJumps)->load();
    glitchParams.keepLooping   = apvts.getRawParameterValue(ParamIDs::loopKeep)->load() > 0.5f;
    glitchParams.holdAmount    = apvts.getRawParameterValue(ParamIDs::glitchHoldAmount)->load();
    glitchParams.degrade       = apvts.getRawParameterValue(ParamIDs::glitchDegrade)->load();
    glitchParams.chaos         = apvts.getRawParameterValue(ParamIDs::glitchChaos)->load();
    glitchParams.reverseAmount = apvts.getRawParameterValue(ParamIDs::glitchReverseAmount)->load();
    glitchParams.toneTilt      = apvts.getRawParameterValue(ParamIDs::glitchToneTilt)->load();

    sidechainGlitch.setParams(glitchParams);
    selfGlitch.setParams(glitchParams);

    WetEngine::Params wetParams;
    wetParams.mode   = (WetEngine::Mode) (int) apvts.getRawParameterValue(ParamIDs::wetMode)->load();
    wetParams.mix    = apvts.getRawParameterValue(ParamIDs::wetMix)->load();
    wetParams.time   = apvts.getRawParameterValue(ParamIDs::wetTime)->load();
    wetParams.modify = apvts.getRawParameterValue(ParamIDs::wetModify)->load();
    wet.setParams(wetParams);
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
    wet.prepare(sampleRate, numChannels, samplesPerBlock);

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

    const bool wetFirst = apvts.getRawParameterValue(ParamIDs::routeOrder)->load() > 0.5f;
    const bool selfGlitchOn = apvts.getRawParameterValue(ParamIDs::selfGlitchEnabled)->load() > 0.5f;

    if (wetFirst)
        wet.process(mainOut);

    if (selfGlitchOn)
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

    if (!wetFirst)
        wet.process(mainOut);

    const float masterGain = juce::Decibels::decibelsToGain(
        apvts.getRawParameterValue(ParamIDs::masterGainDb)->load());
    mainOut.applyGain(masterGain);

    // Soft limiter: loud chords saturate gently instead of hard-clipping (which crackles).
    for (int ch = 0; ch < mainOut.getNumChannels(); ++ch)
    {
        auto* data = mainOut.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
            data[i] = std::tanh(data[i]);
    }
}

juce::AudioProcessorEditor* GlitchSynthAudioProcessor::createEditor()
{
    return new GlitchSynthAudioProcessorEditor(*this);
}

int GlitchSynthAudioProcessor::getNumPrograms()
{
    return (int) getFactoryPresets().size();
}

const juce::String GlitchSynthAudioProcessor::getProgramName(int index)
{
    const auto& presets = getFactoryPresets();
    if (index >= 0 && index < (int) presets.size())
        return presets[(size_t) index].name;
    return {};
}

void GlitchSynthAudioProcessor::setCurrentProgram(int index)
{
    const auto& presets = getFactoryPresets();
    if (index < 0 || index >= (int) presets.size())
        return;

    currentProgramIndex = index;
    currentUserPreset = juce::File();

    // Start from defaults so anything a preset doesn't mention is reset, not left over.
    for (auto* param : getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(param))
            ranged->setValueNotifyingHost(ranged->getDefaultValue());

    for (const auto& presetValue : presets[(size_t) index].values)
    {
        if (auto* param = apvts.getParameter(presetValue.paramID))
        {
            const float normalised = param->getNormalisableRange().convertTo0to1(presetValue.value);
            param->setValueNotifyingHost(normalised);
        }
    }
}

std::unique_ptr<juce::XmlElement> GlitchSynthAudioProcessor::createStateXml()
{
    auto state = apvts.copyState();

    // Custom waves aren't parameters, so they ride along as attributes of the saved state.
    for (int o = 0; o < GlitchSynthVoice::numOscillators; ++o)
        state.setProperty(customWaveStateKey(o), customWaves[(size_t) o].toString(), nullptr);

    return state.createXml();
}

void GlitchSynthAudioProcessor::loadStateXml(const juce::XmlElement& xml)
{
    if (!xml.hasTagName(apvts.state.getType()))
        return;

    for (int o = 0; o < GlitchSynthVoice::numOscillators; ++o)
        if (xml.hasAttribute(customWaveStateKey(o)))
            customWaves[(size_t) o].fromString(xml.getStringAttribute(customWaveStateKey(o)));

    apvts.replaceState(juce::ValueTree::fromXml(xml));
}

void GlitchSynthAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    copyXmlToBinary(*createStateXml(), destData);
}

void GlitchSynthAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        loadStateXml(*xml);
}

namespace
{
    const juce::String userPresetExtension = ".sdcpreset";
}

juce::File GlitchSynthAudioProcessor::getUserPresetFolder()
{
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile("Salir de Casa")
        .getChildFile("Presets");
}

juce::Array<juce::File> GlitchSynthAudioProcessor::getUserPresetFiles()
{
    auto files = getUserPresetFolder().findChildFiles(juce::File::findFiles, false, "*" + userPresetExtension);

    struct ByName
    {
        static int compareElements(const juce::File& a, const juce::File& b)
        {
            return a.getFileNameWithoutExtension().compareNatural(b.getFileNameWithoutExtension());
        }
    } byName;
    files.sort(byName);
    return files;
}

bool GlitchSynthAudioProcessor::saveUserPreset(const juce::String& name)
{
    const auto fileName = juce::File::createLegalFileName(name.trim());
    if (fileName.isEmpty())
        return false;

    const auto folder = getUserPresetFolder();
    if (!folder.createDirectory())
        return false;

    const auto file = folder.getChildFile(fileName + userPresetExtension);
    if (!createStateXml()->writeTo(file))
        return false;

    currentUserPreset = file;
    return true;
}

bool GlitchSynthAudioProcessor::loadUserPreset(const juce::File& file)
{
    auto xml = juce::XmlDocument::parse(file);
    if (xml == nullptr || !xml->hasTagName(apvts.state.getType()))
        return false;

    loadStateXml(*xml);
    currentUserPreset = file;
    return true;
}

juce::String GlitchSynthAudioProcessor::customWaveStateKey(int oscIndex)
{
    return "customWave" + juce::String(oscIndex + 1);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GlitchSynthAudioProcessor();
}
