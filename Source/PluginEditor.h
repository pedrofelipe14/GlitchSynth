#pragma once

#include <array>

#include "PluginProcessor.h"

/** Grid-of-knobs editor, pedal-style: two sections (Sintetizador / Glitch),
    each laid out as rows of rotary knobs rather than one long vertical list. */
class GlitchSynthAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit GlitchSynthAudioProcessorEditor(GlitchSynthAudioProcessor&);
    ~GlitchSynthAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    bool keyPressed(const juce::KeyPress& key) override;
    bool keyStateChanged(bool isKeyDown) override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
        int section = 0; // 0 = Sintetizador, 1 = Glitch
        int row = 0;
        int col = 0;
    };

    Knob& addKnob(const juce::String& paramID, const juce::String& labelText,
                   int section, int row, int col);

    void layoutSection(juce::Rectangle<int> area, int section, int numRows);

    GlitchSynthAudioProcessor& processorRef;

    juce::Label titleLabel { {}, "ANTARTIDA" };
    juce::Label presetLabel { {}, "Preset" };
    juce::ComboBox presetBox;

    juce::Label synthSectionLabel { {}, "SINTETIZADOR" };
    juce::Label glitchSectionLabel { {}, "GLITCH" };

    juce::Label waveformLabel { {}, "Forma de Onda" };
    juce::ComboBox waveformBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveformAttachment;

    juce::ToggleButton selfGlitchButton { "Auto-Glitch" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> selfGlitchAttachment;

    std::vector<std::unique_ptr<Knob>> knobs;

    juce::MidiKeyboardComponent keyboard;

    // Computer-keyboard-to-note mapping (like Logic's Musical Typing / Ableton's
    // computer MIDI keyboard): A S D F G H J K O L P are the white keys, W E T Y
    // are the black keys above them, Z/X shift the octave down/up.
    static constexpr const char* computerKeyboardNoteChars = "AWSEDFTGYHUJKOLP";
    static constexpr int numComputerKeyboardNotes = 16;
    int computerKeyboardBaseNote = 60;
    std::array<bool, numComputerKeyboardNotes> computerKeyDown {};

    static constexpr int cellWidth = 176;
    static constexpr int cellHeight = 100;
    static constexpr int headerHeight = 26;
    static constexpr int topBarHeight = 30;
    static constexpr int sectionGap = 14;
    static constexpr int margin = 16;
    static constexpr int keyboardHeight = 90;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlitchSynthAudioProcessorEditor)
};
