# GlitchSynth

A VST3 synthesizer plugin for FL Studio (and any VST3 host), built with [JUCE](https://juce.com/).

## What it is

GlitchSynth is a subtractive synth (4 waveforms, ADSR, resonant filter with envelope
modulation) that can also take in an audio sidechain and apply glitch processing to it
(bitcrush, tempo-rate stutter/repeat, chunk reversal). The same glitch engine can
optionally be applied to the synth's own output too.

In FL Studio: drop it in the Channel Rack as an instrument, then right-click a mixer
track and "Sidechain to this track" into GlitchSynth's input to feed it audio to glitch.

## Building

Requirements: CMake 3.22+, a C++17 compiler (MSVC on Windows), and the JUCE submodule.

```sh
git submodule update --init --recursive
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

The built VST3 is copied automatically to your system's VST3 folder
(`COPY_PLUGIN_AFTER_BUILD TRUE` in CMakeLists.txt). A Standalone build is also produced
for quick testing without a DAW.

## Project layout

- `Source/PluginProcessor.*` — parameter layout, audio bus setup, processBlock.
- `Source/PluginEditor.*` — UI (currently the generic auto-built parameter editor).
- `Source/DSP/SynthVoice.*` — oscillator + ADSR + filter voice.
- `Source/DSP/GlitchEngine.*` — bitcrush / stutter-repeat / reverse processor.

## Status

v0.1 — functional synth + glitch engine, generic UI. Custom UI is a planned next step.
