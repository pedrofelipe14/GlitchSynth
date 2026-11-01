cmake_minimum_required(VERSION 3.22)

project(Antartida VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/JUCE JUCE)

juce_add_plugin(Antartida
    COMPANY_NAME "YourStudio"
    PLUGIN_MANUFACTURER_CODE Yrst
    PLUGIN_CODE Ant1
    FORMATS VST3 Standalone
    PRODUCT_NAME "Antartida"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    VST3_CATEGORIES Instrument Synth
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_COPY_DIR "D:/VST3Plugins"
)

target_sources(Antartida
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/DSP/SynthVoice.cpp
        Source/DSP/GlitchEngine.cpp
)

target_compile_definitions(Antartida
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(Antartida
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
