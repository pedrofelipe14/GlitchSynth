#pragma once

#include <juce_core/juce_core.h>

namespace ParamIDs
{
    static const juce::String oscWaveform          = "oscWaveform";
    static const juce::String ampAttack             = "ampAttack";
    static const juce::String ampDecay               = "ampDecay";
    static const juce::String ampSustain             = "ampSustain";
    static const juce::String ampRelease             = "ampRelease";
    static const juce::String filterCutoff           = "filterCutoff";
    static const juce::String filterResonance        = "filterResonance";
    static const juce::String filterEnvAmount        = "filterEnvAmount";
    static const juce::String masterGainDb           = "masterGainDb";

    static const juce::String glitchMix              = "glitchMix";
    static const juce::String glitchGrainSizeMs      = "glitchGrainSizeMs";
    static const juce::String glitchPitchRatio       = "glitchPitchRatio";
    static const juce::String glitchHoldAmount       = "glitchHoldAmount";
    static const juce::String glitchDegrade          = "glitchDegrade";
    static const juce::String glitchChaos            = "glitchChaos";
    static const juce::String glitchReverseAmount    = "glitchReverseAmount";
    static const juce::String glitchToneTilt         = "glitchToneTilt";
    static const juce::String selfGlitchEnabled      = "selfGlitchEnabled";
    static const juce::String sidechainLevelDb       = "sidechainLevelDb";
}
