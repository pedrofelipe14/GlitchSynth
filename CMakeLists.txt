cmake_minimum_required(VERSION 3.22)

project(SalirDeCasa VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/JUCE JUCE)

juce_add_plugin(SalirDeCasa
    COMPANY_NAME "Salir de Casa"
    PLUGIN_MANUFACTURER_CODE Sdcs
    PLUGIN_CODE Sdc1
    FORMATS VST3 Standalone
    PRODUCT_NAME "Salir de Casa"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    VST3_CATEGORIES Instrument Synth
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_COPY_DIR "D:/VST3Plugins"
)

target_sources(SalirDeCasa
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/WaveEditor.cpp
        Source/SalirLookAndFeel.cpp
        Source/DSP/SynthVoice.cpp
        Source/DSP/GlitchEngine.cpp
        Source/DSP/WetEngine.cpp
)

juce_add_binary_data(SalirDeCasaAssets
    SOURCES
        Resources/banner.png
)

target_compile_definitions(SalirDeCasa
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(SalirDeCasa
    PRIVATE
        SalirDeCasaAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
