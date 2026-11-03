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

/** Factory presets. Choice values are raw indices:
      oscWaveform, osc2Waveform  0=Seno 1=Sierra 2=Cuadrada 3=Triangular 4=Propia
      wetMode      0=Reverb 1=Delay 2=Slip
      routeOrder   0=Loop>Wet 1=Wet>Loop
      loopInterval 0=-2oct 1=-1oct 2=-5ta 3=-4ta 4=Unisono 5=+4ta 6=+5ta 7=+1oct 8=+1oct+5ta 9=+2oct
    Bool values are 0.0f/1.0f. All other values are the parameter's real range.
    Anything a preset leaves out is reset to its default when the preset loads. */
inline const std::vector<Preset>& getFactoryPresets()
{
    using namespace ParamIDs;

    static const std::vector<Preset> presets = {
        {
            "Init",
            {}
        },
        {
            "Cinta Vieja",
            {
                { oscWaveform, 1 }, { ampAttack, 0.05f }, { ampDecay, 0.3f },
                { ampSustain, 0.6f }, { ampRelease, 0.6f },
                { filterCutoff, 2500.0f }, { filterResonance, 0.5f }, { filterEnvAmount, 12.0f },
                { masterGainDb, -3.0f },
                { wetMode, 1 }, { wetMix, 0.25f }, { wetTime, 0.55f }, { wetModify, 0.35f },
                { glitchMix, 0.6f }, { glitchGrainSizeMs, 400.0f }, { loopInterval, 4 },
                { glitchHoldAmount, 0.6f }, { glitchDegrade, 0.5f }, { glitchChaos, 0.3f },
                { glitchReverseAmount, 0.05f }, { glitchToneTilt, 0.2f },
                { selfGlitchEnabled, 1.0f },
            }
        },
        {
            "Loop Congelado",
            {
                { oscWaveform, 0 }, { ampAttack, 1.2f }, { ampDecay, 1.0f },
                { ampSustain, 1.0f }, { ampRelease, 2.0f },
                { filterCutoff, 6000.0f }, { filterResonance, 0.3f }, { filterEnvAmount, 6.0f },
                { masterGainDb, -2.0f },
                { wetMode, 0 }, { wetMix, 0.45f }, { wetTime, 0.8f }, { wetModify, 0.6f },
                { glitchMix, 0.8f }, { glitchGrainSizeMs, 600.0f }, { loopInterval, 4 },
                { glitchHoldAmount, 0.9f }, { glitchDegrade, 0.15f }, { glitchChaos, 0.4f },
                { glitchReverseAmount, 0.2f }, { glitchToneTilt, 0.6f },
                { selfGlitchEnabled, 1.0f }, { loopKeep, 1.0f },
            }
        },
        {
            "Caos Total",
            {
                { oscWaveform, 1 }, { ampAttack, 0.01f }, { ampDecay, 0.2f },
                { ampSustain, 0.5f }, { ampRelease, 0.3f },
                { filterCutoff, 5000.0f }, { filterResonance, 2.0f }, { filterEnvAmount, 36.0f },
                { masterGainDb, -4.0f },
                { wetMode, 2 }, { wetMix, 0.35f }, { wetTime, 0.4f }, { wetModify, 0.1f },
                { glitchMix, 1.0f }, { glitchGrainSizeMs, 150.0f }, { loopInterval, 4 },
                { loopJumps, 0.6f },
                { glitchHoldAmount, 0.4f }, { glitchDegrade, 0.8f }, { glitchChaos, 0.9f },
                { glitchReverseAmount, 0.5f }, { glitchToneTilt, 0.5f },
                { selfGlitchEnabled, 1.0f },
            }
        },
        {
            "Glitch Suave",
            {
                { oscWaveform, 1 }, { ampAttack, 0.02f }, { ampDecay, 0.15f },
                { ampSustain, 0.7f }, { ampRelease, 0.25f },
                { filterCutoff, 5500.0f }, { filterResonance, 0.6f }, { filterEnvAmount, 18.0f },
                { wetMode, 0 }, { wetMix, 0.25f }, { wetTime, 0.5f }, { wetModify, 0.5f },
                { glitchMix, 0.25f }, { glitchGrainSizeMs, 300.0f }, { loopInterval, 4 },
                { loopJumps, 0.15f },
                { glitchHoldAmount, 0.2f }, { glitchDegrade, 0.15f }, { glitchChaos, 0.1f },
                { glitchReverseAmount, 0.05f }, { glitchToneTilt, 0.55f },
                { selfGlitchEnabled, 1.0f },
            }
        },
        {
            "Lead Roto",
            {
                { oscWaveform, 2 }, { ampAttack, 0.005f }, { ampDecay, 0.08f },
                { ampSustain, 0.9f }, { ampRelease, 0.1f },
                { filterCutoff, 3000.0f }, { filterResonance, 3.0f }, { filterEnvAmount, 30.0f },
                { masterGainDb, -1.0f },
                { wetMode, 1 }, { wetMix, 0.2f }, { wetTime, 0.45f }, { wetModify, 0.3f },
                { glitchMix, 0.35f }, { glitchGrainSizeMs, 120.0f }, { loopInterval, 6 },
                { loopJumps, 0.25f },
                { glitchHoldAmount, 0.15f }, { glitchDegrade, 0.3f }, { glitchChaos, 0.35f },
                { glitchReverseAmount, 0.1f }, { glitchToneTilt, 0.6f },
                { selfGlitchEnabled, 1.0f },
            }
        },
        {
            "Pad Fantasma",
            {
                { oscWaveform, 0 }, { ampAttack, 2.0f }, { ampDecay, 1.5f },
                { ampSustain, 1.0f }, { ampRelease, 3.0f },
                { filterCutoff, 8000.0f }, { filterResonance, 0.2f }, { filterEnvAmount, 4.0f },
                { masterGainDb, -3.0f },
                { wetMode, 0 }, { wetMix, 0.6f }, { wetTime, 0.95f }, { wetModify, 0.7f },
                { routeOrder, 1 },
                { glitchMix, 0.5f }, { glitchGrainSizeMs, 800.0f }, { loopInterval, 1 },
                { glitchHoldAmount, 0.95f }, { glitchDegrade, 0.1f }, { glitchChaos, 0.5f },
                { glitchReverseAmount, 0.6f }, { glitchToneTilt, 0.7f },
                { selfGlitchEnabled, 1.0f },
            }
        },
        {
            "Percusion Glitch",
            {
                { oscWaveform, 3 }, { ampAttack, 0.001f }, { ampDecay, 0.05f },
                { ampSustain, 0.0f }, { ampRelease, 0.05f },
                { filterCutoff, 7000.0f }, { filterResonance, 1.0f }, { filterEnvAmount, 24.0f },
                { wetMode, 1 }, { wetMix, 0.2f }, { wetTime, 0.3f }, { wetModify, 0.45f },
                { glitchMix, 0.7f }, { glitchGrainSizeMs, 60.0f }, { loopInterval, 7 },
                { loopJumps, 0.3f },
                { glitchHoldAmount, 0.1f }, { glitchDegrade, 0.6f }, { glitchChaos, 0.4f },
                { glitchReverseAmount, 0.3f }, { glitchToneTilt, 0.5f },
                { selfGlitchEnabled, 1.0f },
            }
        },
        {
            "Reversa Eterna",
            {
                { oscWaveform, 3 }, { ampAttack, 0.3f }, { ampDecay, 0.8f },
                { ampSustain, 0.8f }, { ampRelease, 1.2f },
                { filterCutoff, 4500.0f }, { filterResonance, 0.4f }, { filterEnvAmount, 8.0f },
                { masterGainDb, -2.0f },
                { wetMode, 2 }, { wetMix, 0.55f }, { wetTime, 0.7f }, { wetModify, 0.1f },
                { routeOrder, 1 },
                { glitchMix, 0.3f }, { glitchGrainSizeMs, 500.0f }, { loopInterval, 7 },
                { glitchHoldAmount, 0.7f }, { glitchDegrade, 0.05f }, { glitchChaos, 0.2f },
                { glitchReverseAmount, 0.8f }, { glitchToneTilt, 0.6f },
                { selfGlitchEnabled, 1.0f },
            }
        },
        {
            "Eco Polar",
            {
                { oscWaveform, 1 }, { ampAttack, 0.01f }, { ampDecay, 0.4f },
                { ampSustain, 0.3f }, { ampRelease, 0.4f },
                { filterCutoff, 1800.0f }, { filterResonance, 1.2f }, { filterEnvAmount, 30.0f },
                { masterGainDb, -2.0f },
                { wetMode, 1 }, { wetMix, 0.5f }, { wetTime, 0.62f }, { wetModify, 0.7f },
                { glitchMix, 0.2f }, { glitchGrainSizeMs, 200.0f }, { loopInterval, 7 },
                { loopJumps, 0.4f },
                { glitchHoldAmount, 0.3f }, { glitchDegrade, 0.2f }, { glitchChaos, 0.25f },
                { glitchReverseAmount, 0.2f }, { glitchToneTilt, 0.7f },
                { selfGlitchEnabled, 1.0f },
            }
        },
    };

    return presets;
}
