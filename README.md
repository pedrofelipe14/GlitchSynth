# GlitchSynth

A VST3 synthesizer plugin for FL Studio (and any VST3 host), built with [JUCE](https://juce.com/).

## What it is

GlitchSynth is a subtractive synth (4 waveforms, ADSR, resonant filter with envelope
modulation) paired with a Chase Bliss "Mood"/"Blooper"-style granular degrader that can
run on an audio sidechain input, the synth's own output, or both. It continuously
records live audio, freezes grains and loops them, pitch-shifts the loop, and
progressively degrades it (bit/rate crush + grit) the longer a grain repeats. A single
"Chaos" knob drives generative randomness on top: tape-style wow/flutter, random reverse
flips, and random decisions to grab fresh material vs. keep looping what it already has
— aimed at glitchcore/indietronica textures (think Eartheater, THE HELLP).

Glitch params: Grain Size, Pitch, Hold (freeze-loop tendency), Degrade, Chaos, Reverse
Amount, and Tone Tilt (dark/lowpassed to bright/highpass-emphasised).

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
