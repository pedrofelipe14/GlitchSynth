#pragma once

#include <juce_core/juce_core.h>
#include <array>

namespace ParamIDs
{
    // Oscillator 1 keeps the original "oscWaveform" ID so older sessions still load.
    static const juce::String oscWaveform          = "oscWaveform";
    static const juce::String osc1Level             = "osc1Level";
    static const juce::String osc1Octave            = "osc1Octave";
    static const juce::String osc1Semitone          = "osc1Semitone";
    static const juce::String osc1Fine              = "osc1Fine";
    static const juce::String osc2Waveform          = "osc2Waveform";
    static const juce::String osc2Level             = "osc2Level";
    static const juce::String osc2Octave            = "osc2Octave";
    static const juce::String osc2Semitone          = "osc2Semitone";
    static const juce::String osc2Fine              = "osc2Fine";

    struct OscillatorIDs
    {
        juce::String waveform, level, octave, semitone, fine;
    };

    static const std::array<OscillatorIDs, 2> oscillators {{
        { oscWaveform,  osc1Level, osc1Octave, osc1Semitone, osc1Fine },
        { osc2Waveform, osc2Level, osc2Octave, osc2Semitone, osc2Fine },
    }};

    static const juce::String ampAttack             = "ampAttack";
    static const juce::String ampDecay               = "ampDecay";
    static const juce::String ampSustain             = "ampSustain";
    static const juce::String ampRelease             = "ampRelease";
    static const juce::String filterCutoff           = "filterCutoff";
    static const juce::String filterResonance        = "filterResonance";
    static const juce::String filterEnvAmount        = "filterEnvAmount";
    static const juce::String masterGainDb           = "masterGainDb";
    static const juce::String pitchBendRange         = "pitchBendRange";

    static const juce::String lfoShape               = "lfoShape";
    static const juce::String lfoTarget              = "lfoTarget";
    static const juce::String lfoRate                = "lfoRate";
    static const juce::String lfoDepth               = "lfoDepth";

    static const juce::String wetMode                = "wetMode";
    static const juce::String wetMix                 = "wetMix";
    static const juce::String wetTime                = "wetTime";
    static const juce::String wetModify              = "wetModify";
    static const juce::String routeOrder             = "routeOrder";

    static const juce::String glitchMix              = "glitchMix";
    static const juce::String glitchGrainSizeMs      = "glitchGrainSizeMs";
    static const juce::String loopInterval           = "loopInterval";
    static const juce::String loopJumps              = "loopJumps";
    static const juce::String glitchHoldAmount       = "glitchHoldAmount";
    static const juce::String glitchDegrade          = "glitchDegrade";
    static const juce::String glitchChaos            = "glitchChaos";
    static const juce::String glitchReverseAmount    = "glitchReverseAmount";
    static const juce::String glitchToneTilt         = "glitchToneTilt";
    static const juce::String selfGlitchEnabled      = "selfGlitchEnabled";
    static const juce::String loopKeep               = "loopKeep";
    static const juce::String sidechainLevelDb       = "sidechainLevelDb";
}
