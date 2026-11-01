#include "PluginEditor.h"
#include "ParamIDs.h"
#include "Presets.h"

GlitchSynthAudioProcessorEditor::GlitchSynthAudioProcessorEditor(GlitchSynthAudioProcessor& p)
    : AudioProcessorEditor(&p),
      processorRef(p),
      keyboard(p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    keyboard.setLowestVisibleKey(48);
    keyboard.setAvailableRange(0, 127);
    keyboard.setWantsKeyboardFocus(false); // keep computer-keyboard focus on the editor itself
    addAndMakeVisible(keyboard);

    titleLabel.setFont(juce::Font(juce::FontOptions(20.0f, juce::Font::bold)));
    addAndMakeVisible(titleLabel);

    presetLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(presetLabel);

    const auto& presets = getFactoryPresets();
    for (int i = 0; i < (int) presets.size(); ++i)
        presetBox.addItem(presets[(size_t) i].name, i + 1);
    presetBox.setSelectedId(processorRef.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this]
    {
        processorRef.setCurrentProgram(presetBox.getSelectedId() - 1);
    };
    addAndMakeVisible(presetBox);

    synthSectionLabel.setJustificationType(juce::Justification::centredLeft);
    synthSectionLabel.setFont(juce::Font(juce::FontOptions(18.0f, juce::Font::bold)));
    addAndMakeVisible(synthSectionLabel);

    glitchSectionLabel.setJustificationType(juce::Justification::centredLeft);
    glitchSectionLabel.setFont(juce::Font(juce::FontOptions(18.0f, juce::Font::bold)));
    addAndMakeVisible(glitchSectionLabel);

    waveformLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(waveformLabel);

    waveformBox.addItemList({ "Seno", "Sierra", "Cuadrada", "Triangular" }, 1);
    addAndMakeVisible(waveformBox);
    waveformAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processorRef.apvts, ParamIDs::oscWaveform, waveformBox);

    addAndMakeVisible(selfGlitchButton);
    selfGlitchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.apvts, ParamIDs::selfGlitchEnabled, selfGlitchButton);

    // Sintetizador, fila A
    addKnob(ParamIDs::ampAttack,          "Ataque",         0, 0, 1);
    addKnob(ParamIDs::ampDecay,           "Decaimiento",    0, 0, 2);
    addKnob(ParamIDs::ampSustain,         "Sustain",        0, 0, 3);
    addKnob(ParamIDs::ampRelease,         "Release",        0, 0, 4);

    // Sintetizador, fila B
    addKnob(ParamIDs::filterCutoff,       "Corte",          0, 1, 0);
    addKnob(ParamIDs::filterResonance,    "Resonancia",     0, 1, 1);
    addKnob(ParamIDs::filterEnvAmount,    "Env. Filtro",    0, 1, 2);
    addKnob(ParamIDs::masterGainDb,       "Volumen",        0, 1, 3);

    // Glitch, fila A
    addKnob(ParamIDs::glitchMix,          "Mezcla",         1, 0, 0);
    addKnob(ParamIDs::glitchGrainSizeMs,  "Tam. Grano",     1, 0, 1);
    addKnob(ParamIDs::glitchPitchRatio,   "Tono Loop",      1, 0, 2);
    addKnob(ParamIDs::glitchHoldAmount,   "Congelar",       1, 0, 3);
    addKnob(ParamIDs::sidechainLevelDb,   "Nivel SC",       1, 0, 4);

    // Glitch, fila B
    addKnob(ParamIDs::glitchDegrade,      "Degradacion",    1, 1, 0);
    addKnob(ParamIDs::glitchChaos,        "Caos",           1, 1, 1);
    addKnob(ParamIDs::glitchReverseAmount,"Reversa",        1, 1, 2);
    addKnob(ParamIDs::glitchToneTilt,     "Tilt",           1, 1, 3);

    setSize(5 * cellWidth + 2 * margin,
            topBarHeight + 2 * headerHeight + 4 * cellHeight + sectionGap + 2 * margin + keyboardHeight + margin);
    setResizable(true, true);

    setWantsKeyboardFocus(true);
    grabKeyboardFocus();
}

GlitchSynthAudioProcessorEditor::~GlitchSynthAudioProcessorEditor() = default;

GlitchSynthAudioProcessorEditor::Knob& GlitchSynthAudioProcessorEditor::addKnob(
    const juce::String& paramID, const juce::String& labelText, int section, int row, int col)
{
    auto knob = std::make_unique<Knob>();
    knob->section = section;
    knob->row = row;
    knob->col = col;

    knob->slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    knob->slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 18);
    addAndMakeVisible(knob->slider);

    knob->label.setText(labelText, juce::dontSendNotification);
    knob->label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(knob->label);

    knob->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.apvts, paramID, knob->slider);

    knobs.push_back(std::move(knob));
    return *knobs.back();
}

void GlitchSynthAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void GlitchSynthAudioProcessorEditor::layoutSection(juce::Rectangle<int> area, int section, int numRows)
{
    for (auto& knob : knobs)
    {
        if (knob->section != section)
            continue;

        auto cell = area.withX(area.getX() + knob->col * cellWidth)
                        .withY(area.getY() + knob->row * cellHeight)
                        .withWidth(cellWidth)
                        .withHeight(cellHeight);

        knob->label.setBounds(cell.removeFromTop(18));
        knob->slider.setBounds(cell.reduced(6, 0));
    }

    juce::ignoreUnused(numRows);
}

void GlitchSynthAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(margin);

    auto keyboardArea = bounds.removeFromBottom(keyboardHeight);
    keyboard.setBounds(keyboardArea);
    bounds.removeFromBottom(margin / 2);

    auto topBar = bounds.removeFromTop(topBarHeight);
    titleLabel.setBounds(topBar.removeFromLeft(3 * cellWidth / 2));
    presetBox.setBounds(topBar.removeFromRight(2 * cellWidth));
    presetLabel.setBounds(topBar.removeFromRight(cellWidth / 2));
    bounds.removeFromTop(6);

    auto synthHeader = bounds.removeFromTop(headerHeight);
    synthSectionLabel.setBounds(synthHeader);
    waveformLabel.setBounds(synthHeader.withX(synthHeader.getX() + 4 * cellWidth).withWidth(cellWidth / 2));
    waveformBox.setBounds(synthHeader.withX(synthHeader.getX() + 4 * cellWidth + cellWidth / 2)
                                      .withWidth(cellWidth / 2).reduced(0, 1));

    auto synthArea = bounds.removeFromTop(2 * cellHeight);
    layoutSection(synthArea, 0, 2);

    bounds.removeFromTop(sectionGap);

    auto glitchHeader = bounds.removeFromTop(headerHeight);
    glitchSectionLabel.setBounds(glitchHeader);
    selfGlitchButton.setBounds(glitchHeader.withX(glitchHeader.getX() + 3 * cellWidth).withWidth(2 * cellWidth));

    auto glitchArea = bounds.removeFromTop(2 * cellHeight);
    layoutSection(glitchArea, 1, 2);
}

bool GlitchSynthAudioProcessorEditor::keyPressed(const juce::KeyPress& key)
{
    const auto c = key.getTextCharacter();

    if (c == 'z' || c == 'Z')
    {
        computerKeyboardBaseNote = juce::jmax(0, computerKeyboardBaseNote - 12);
        return true;
    }

    if (c == 'x' || c == 'X')
    {
        computerKeyboardBaseNote = juce::jmin(127 - numComputerKeyboardNotes, computerKeyboardBaseNote + 12);
        return true;
    }

    return false;
}

bool GlitchSynthAudioProcessorEditor::keyStateChanged(bool /*isKeyDown*/)
{
    bool handledAny = false;

    for (int i = 0; i < numComputerKeyboardNotes; ++i)
    {
        const bool down = juce::KeyPress::isKeyCurrentlyDown((int) computerKeyboardNoteChars[i]);

        if (down != computerKeyDown[(size_t) i])
        {
            computerKeyDown[(size_t) i] = down;
            handledAny = true;

            const int noteNumber = juce::jlimit(0, 127, computerKeyboardBaseNote + i);

            if (down)
                processorRef.keyboardState.noteOn(1, noteNumber, 1.0f);
            else
                processorRef.keyboardState.noteOff(1, noteNumber, 0.0f);
        }
    }

    return handledAny;
}
