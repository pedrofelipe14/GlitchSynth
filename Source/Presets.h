#pragma once

#include <juce_core/juce_core.h>
#include <vector>

#include "ParamIDs.h"

struct PresetValue
{
    juce::String paramID;
    float value;
};

struct Preset
{
    juce::String name;
    std::vector<PresetValue> values;
};

/** Factory presets. Waveform values are raw choice indices (0=Seno, 1=Sierra,
    2=Cuadrada, 3=Triangular); bool values are 0.0f/1.0f. All other values are
    the parameter's real (non-normalised) range. */
inline const std::vector<Preset>& getFactoryPresets()
{
    using namespace ParamIDs;

    static const std::vector<Preset> presets = {
        {
            "Init",
            {
                { oscWaveform, 1 }, { ampAttack, 0.01f }, { ampDecay, 0.1f },
                { ampSustain, 0.8f }, { ampRelease, 0.2f },
                { filterCutoff, 4000.0f }, { filterResonance, 0.7f }, { filterEnvAmount, 24.0f },
                { masterGainDb, 0.0f },
                { glitchMix, 0.0f }, { glitchGrainSizeMs, 250.0f }, { glitchPitchRatio, 1.0f },
                { glitchHoldAmount, 0.3f }, { glitchDegrade, 0.2f }, { glitchChaos, 0.2f },
                { glitchReverseAmount, 0.1f }, { glitchToneTilt, 0.5f },
                { selfGlitchEnabled, 0.0f }, { sidechainLevelDb, 0.0f },
            }
        },
        {
            "Cinta Vieja",
            {
                { oscWaveform, 1 }, { ampAttack, 0.05f }, { ampDecay, 0.3f },
                { ampSustain, 0.6f }, { ampRelease, 0.6f },
                { filterCutoff, 2500.0f }, { filterResonance, 0.5f }, { filterEnvAmount, 12.0f },
                { masterGainDb, -3.0f },
                { glitchMix, 0.6f }, { glitchGrainSizeMs, 400.0f }, { glitchPitchRatio, 1.0f },
                { glitchHoldAmount, 0.6f }, { glitchDegrade, 0.5f }, { glitchChaos, 0.3f },
                { glitchReverseAmount, 0.05f }, { glitchToneTilt, 0.2f },
                { selfGlitchEnabled, 1.0f }, { sidechainLevelDb, 0.0f },
            }
        },
        {
            "Loop Congelado",
            {
                { oscWaveform, 0 }, { ampAttack, 1.2f }, { ampDecay, 1.0f },
                { ampSustain, 1.0f }, { ampRelease, 2.0f },
                { filterCutoff, 6000.0f }, { filterResonance, 0.3f }, { filterEnvAmount, 6.0f },
                { masterGainDb, -2.0f },
                { glitchMix, 0.8f }, { glitchGrainSizeMs, 600.0f }, { glitchPitchRatio, 1.0f },
                { glitchHoldAmount, 0.9f }, { glitchDegrade, 0.15f }, { glitchChaos, 0.4f },
                { glitchReverseAmount, 0.2f }, { glitchToneTilt, 0.6f },
                { selfGlitchEnabled, 1.0f }, { sidechainLevelDb, 0.0f },
            }
        },
        {
            "Caos Total",
            {
                { oscWaveform, 1 }, { ampAttack, 0.01f }, { ampDecay, 0.2f },
                { ampSustain, 0.5f }, { ampRelease, 0.3f },
                { filterCutoff, 5000.0f }, { filterResonance, 2.0f }, { filterEnvAmount, 36.0f },
                { masterGainDb, -4.0f },
                { glitchMix, 1.0f }, { glitchGrainSizeMs, 150.0f }, { glitchPitchRatio, 1.3f },
                { glitchHoldAmount, 0.4f }, { glitchDegrade, 0.8f }, { glitchChaos, 0.9f },
                { glitchReverseAmount, 0.5f }, { glitchToneTilt, 0.5f },
                { selfGlitchEnabled, 1.0f }, { sidechainLevelDb, 0.0f },
            }
        },
        {
            "Glitch Suave",
            {
                { oscWaveform, 1 }, { ampAttack, 0.02f }, { ampDecay, 0.15f },
                { ampSustain, 0.7f }, { ampRelease, 0.25f },
                { filterCutoff, 5500.0f }, { filterResonance, 0.6f }, { filterEnvAmount, 18.0f },
                { masterGainDb, 0.0f },
                { glitchMix, 0.25f }, { glitchGrainSizeMs, 300.0f }, { glitchPitchRatio, 1.0f },
                { glitchHoldAmount, 0.2f }, { glitchDegrade, 0.15f }, { glitchChaos, 0.1f },
                { glitchReverseAmount, 0.05f }, { glitchToneTilt, 0.55f },
                { selfGlitchEnabled, 1.0f }, { sidechainLevelDb, 0.0f },
            }
        },
        {
            "Lead Roto",
            {
                { oscWaveform, 2 }, { ampAttack, 0.005f }, { ampDecay, 0.08f },
                { ampSustain, 0.9f }, { ampRelease, 0.1f },
                { filterCutoff, 3000.0f }, { filterResonance, 3.0f }, { filterEnvAmount, 30.0f },
                { masterGainDb, -1.0f },
                { glitchMix, 0.35f }, { glitchGrainSizeMs, 120.0f }, { glitchPitchRatio, 1.5f },
                { glitchHoldAmount, 0.15f }, { glitchDegrade, 0.3f }, { glitchChaos, 0.35f },
                { glitchReverseAmount, 0.1f }, { glitchToneTilt, 0.6f },
                { selfGlitchEnabled, 1.0f }, { sidechainLevelDb, 0.0f },
            }
        },
        {
            "Pad Fantasma",
            {
                { oscWaveform, 0 }, { ampAttack, 2.0f }, { ampDecay, 1.5f },
                { ampSustain, 1.0f }, { ampRelease, 3.0f },
                { filterCutoff, 8000.0f }, { filterResonance, 0.2f }, { filterEnvAmount, 4.0f },
                { masterGainDb, -3.0f },
                { glitchMix, 0.5f }, { glitchGrainSizeMs, 800.0f }, { glitchPitchRatio, 0.5f },
                { glitchHoldAmount, 0.95f }, { glitchDegrade, 0.1f }, { glitchChaos, 0.5f },
                { glitchReverseAmount, 0.6f }, { glitchToneTilt, 0.7f },
                { selfGlitchEnabled, 1.0f }, { sidechainLevelDb, 0.0f },
            }
        },
        {
            "Percusion Glitch",
            {
                { oscWaveform, 3 }, { ampAttack, 0.001f }, { ampDecay, 0.05f },
                { ampSustain, 0.0f }, { ampRelease, 0.05f },
                { filterCutoff, 7000.0f }, { filterResonance, 1.0f }, { filterEnvAmount, 24.0f },
                { masterGainDb, 0.0f },
                { glitchMix, 0.7f }, { glitchGrainSizeMs, 60.0f }, { glitchPitchRatio, 2.0f },
                { glitchHoldAmount, 0.1f }, { glitchDegrade, 0.6f }, { glitchChaos, 0.4f },
                { glitchReverseAmount, 0.3f }, { glitchToneTilt, 0.5f },
                { selfGlitchEnabled, 0.0f }, { sidechainLevelDb, 0.3f },
            }
        },
    };

    return presets;
}
