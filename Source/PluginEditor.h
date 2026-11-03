#pragma once

#include <array>
#include <map>

#include "PluginProcessor.h"
#include "SalirLookAndFeel.h"
#include "WaveEditor.h"

/** Pedal-style editor: a synth section across the top (one row per oscillator,
    each with a shapeable waveform, then envelope/filter), then LFO, WET and
    MICRO-LOOP side by side, with the on-screen keyboard at the bottom. */
class GlitchSynthAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    explicit GlitchSynthAudioProcessorEditor(GlitchSynthAudioProcessor&);
    ~GlitchSynthAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    bool keyPressed(const juce::KeyPress& key) override;
    bool keyStateChanged(bool isKeyDown) override;
    void focusLost(FocusChangeType) override;

private:
    enum Section { synthSection = 0, lfoSection, wetSection, loopSection, numSections };

    /** One grid cell: a label over a knob, a drop-down or a switch. */
    struct Control
    {
        juce::Label label;
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::ComboBox> combo;
        std::unique_ptr<juce::ToggleButton> toggle;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> comboAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> toggleAttachment;
        Section section = synthSection;
        int row = 0;
        int col = 0;
    };

    void addKnob(const juce::String& paramID, const juce::String& labelText,
                  Section section, int row, int col);
    void addChoice(const juce::String& paramID, const juce::String& labelText,
                    Section section, int row, int col);
    void addToggle(const juce::String& paramID, const juce::String& labelText,
                    Section section, int row, int col);
    Control& addControl(const juce::String& labelText, Section section, int row, int col);

    void timerCallback() override;
    int computerKeyIndexFor(juce::juce_wchar c) const;
    void releaseComputerKey(int index);
    void releaseAllComputerKeys();

    static int numWhiteKeys();

    GlitchSynthAudioProcessor& processorRef;
    SalirLookAndFeel lookAndFeel;

    // The banner image is the title; it's painted (not a component) inside bannerArea.
    juce::Image banner;
    juce::Rectangle<int> bannerArea;
    juce::Label presetLabel { {}, "PRESET" };
    juce::ComboBox presetBox;

    std::array<juce::Label, numSections> sectionLabels;
    std::array<juce::Rectangle<int>, numSections> sectionPanels;
    std::array<juce::Rectangle<int>, numSections> sectionGrids;

    std::vector<std::unique_ptr<Control>> controls;
    std::array<std::unique_ptr<WaveEditor>, GlitchSynthVoice::numOscillators> waveEditors;

    juce::MidiKeyboardComponent keyboard;

    // Computer-keyboard-to-note mapping (like Logic's Musical Typing / Ableton's
    // computer MIDI keyboard): A S D F G H J K O L P are the white keys, W E T Y
    // are the black keys above them, Z/X shift the octave down/up.
    static constexpr const char* computerKeyboardNoteChars = "AWSEDFTGYHUJKOLP";
    static constexpr int numComputerKeyboardNotes = 16;
    int computerKeyboardBaseNote = 60;
    // The note each held key started (-1 = not held). Note-off goes to this note,
    // not to one recomputed from the current octave, so Z/X mid-note can't strand it.
    std::array<int, numComputerKeyboardNotes> computerKeyNote {};

    static constexpr int lowestKeyboardNote = 36;  // C2
    static constexpr int highestKeyboardNote = 96; // C7

    // One grid cell, top to bottom: label, gap, knob, value read-out, breathing room.
    static constexpr int cellWidth = 106;
    static constexpr int labelHeight = 14;
    static constexpr int knobSize = 60;
    static constexpr int cellHeight = labelHeight + 4 + knobSize + SalirLookAndFeel::valueTextHeight + 14;

    static constexpr int headerHeight = 34;
    static constexpr int topBarHeight = 84;
    static constexpr int sectionGap = 14;
    static constexpr int panelPadding = 16;
    static constexpr int margin = 20;
    static constexpr int keyboardHeight = 72;

    static constexpr int synthCols = 9;
    static constexpr int synthRows = GlitchSynthVoice::numOscillators + 1;
    static constexpr int waveEditorCols = 3; // grid cells each wave editor spans
    static constexpr int lfoCols = 2;
    static constexpr int wetCols = 3;
    static constexpr int loopCols = 6;
    static constexpr int effectRows = 2;

    // Everything is laid out once at this "design" size, then the whole UI is
    // scaled uniformly to the window, so shrinking never squashes or overlaps it.
    int baseWidth = 0;
    int baseHeight = 0;
    float uiScale = 1.0f;
    std::vector<juce::Component*> scaledComponents;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlitchSynthAudioProcessorEditor)
};
