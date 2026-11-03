#include "PluginEditor.h"
#include "ParamIDs.h"
#include "Presets.h"
#include "Theme.h"
#include "BinaryData.h"

GlitchSynthAudioProcessorEditor::GlitchSynthAudioProcessorEditor(GlitchSynthAudioProcessor& p)
    : AudioProcessorEditor(&p),
      processorRef(p),
      keyboard(p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel(&lookAndFeel);

    // C2..C7, numbered the way FL Studio does (middle C = C5).
    keyboard.setAvailableRange(lowestKeyboardNote, highestKeyboardNote);
    keyboard.setLowestVisibleKey(lowestKeyboardNote);
    keyboard.setOctaveForMiddleC(5);
    keyboard.setScrollButtonsVisible(false);
    keyboard.setWantsKeyboardFocus(false); // keep computer-keyboard focus on the editor itself
    addAndMakeVisible(keyboard);

    banner = juce::ImageCache::getFromMemory(BinaryData::banner_png, BinaryData::banner_pngSize);

    presetLabel.setFont(SalirLookAndFeel::labelFont());
    presetLabel.setColour(juce::Label::textColourId, Theme::textDim);
    presetLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(presetLabel);

    refreshPresetList();
    presetBox.setWantsKeyboardFocus(false);
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id >= firstUserPresetId)
        {
            const int index = id - firstUserPresetId;
            if (index < userPresetFiles.size())
                processorRef.loadUserPreset(userPresetFiles[index]);
        }
        else if (id > 0)
        {
            processorRef.setCurrentProgram(id - 1);
        }
    };
    addAndMakeVisible(presetBox);

    savePresetButton.setWantsKeyboardFocus(false);
    savePresetButton.setMouseClickGrabsKeyboardFocus(false); // or clicking it would cancel the name field
    savePresetButton.onClick = [this]
    {
        if (presetNameEditor.isVisible())
            finishSavingPreset(true);
        else
            beginSavingPreset();
    };
    addAndMakeVisible(savePresetButton);

    presetNameEditor.setFont(juce::Font(juce::FontOptions(13.0f)));
    presetNameEditor.setTextToShowWhenEmpty("nombre del preset", Theme::textDim.withAlpha(0.6f));
    presetNameEditor.setIndents(10, 7);
    presetNameEditor.onReturnKey = [this] { finishSavingPreset(true); };
    presetNameEditor.onEscapeKey = [this] { finishSavingPreset(false); };
    presetNameEditor.onFocusLost = [this] { finishSavingPreset(false); };
    addChildComponent(presetNameEditor);

    const juce::StringArray sectionNames { "SINTETIZADOR", "LFO", "WET", "MICRO-LOOP" };
    for (int s = 0; s < numSections; ++s)
    {
        auto& label = sectionLabels[(size_t) s];
        label.setText(sectionNames[s], juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centredLeft);
        label.setFont(SalirLookAndFeel::sectionFont());
        label.setColour(juce::Label::textColourId, Theme::accent);
        addAndMakeVisible(label);
    }

    // Sintetizador: one row per oscillator (wave editor, then its controls)...
    for (int i = 0; i < GlitchSynthVoice::numOscillators; ++i)
    {
        const auto& ids = ParamIDs::oscillators[(size_t) i];

        auto& waveEditor = waveEditors[(size_t) i];
        waveEditor = std::make_unique<WaveEditor>(processorRef, i, ids.waveform, "OSC " + juce::String(i + 1));
        addAndMakeVisible(*waveEditor);

        addChoice(ids.waveform,             "Forma",          synthSection, i, waveEditorCols);
        addKnob(ids.level,                  "Nivel",          synthSection, i, waveEditorCols + 1);
        addKnob(ids.octave,                 "Octava",         synthSection, i, waveEditorCols + 2);
        addKnob(ids.semitone,               "Semitonos",      synthSection, i, waveEditorCols + 3);
        addKnob(ids.fine,                   "Fino",           synthSection, i, waveEditorCols + 4);
    }

    // ...then envelope, filter, pitch bend and volume.
    const int envRow = GlitchSynthVoice::numOscillators;
    addKnob(ParamIDs::ampAttack,            "Ataque",         synthSection, envRow, 0);
    addKnob(ParamIDs::ampDecay,             "Decaimiento",    synthSection, envRow, 1);
    addKnob(ParamIDs::ampSustain,           "Sustain",        synthSection, envRow, 2);
    addKnob(ParamIDs::ampRelease,           "Release",        synthSection, envRow, 3);
    addKnob(ParamIDs::filterCutoff,         "Corte",          synthSection, envRow, 4);
    addKnob(ParamIDs::filterResonance,      "Resonancia",     synthSection, envRow, 5);
    addKnob(ParamIDs::filterEnvAmount,      "Env. Filtro",    synthSection, envRow, 6);
    addKnob(ParamIDs::pitchBendRange,       "Rango Bend",     synthSection, envRow, 7);
    addKnob(ParamIDs::masterGainDb,         "Volumen",        synthSection, envRow, 8);

    // LFO
    addChoice(ParamIDs::lfoShape,           "Forma",          lfoSection, 0, 0);
    addChoice(ParamIDs::lfoTarget,          "Destino",        lfoSection, 0, 1);
    addKnob(ParamIDs::lfoRate,              "Velocidad",      lfoSection, 1, 0);
    addKnob(ParamIDs::lfoDepth,             "Profundidad",    lfoSection, 1, 1);

    // Wet
    addKnob(ParamIDs::wetMix,              "Mezcla",         wetSection, 0, 0);
    addKnob(ParamIDs::wetTime,              "Tiempo",         wetSection, 0, 1);
    addKnob(ParamIDs::wetModify,            "Modificar",      wetSection, 0, 2);
    addChoice(ParamIDs::wetMode,            "Modo",           wetSection, 1, 0);
    addChoice(ParamIDs::routeOrder,         "Orden",          wetSection, 1, 1);

    // Micro-loop, fila A
    addKnob(ParamIDs::glitchMix,            "Mezcla",         loopSection, 0, 0);
    addKnob(ParamIDs::glitchGrainSizeMs,    "Largo",          loopSection, 0, 1);
    addChoice(ParamIDs::loopInterval,       "Intervalo",      loopSection, 0, 2);
    addKnob(ParamIDs::loopJumps,            "Saltos",         loopSection, 0, 3);
    addKnob(ParamIDs::glitchHoldAmount,     "Repetir",        loopSection, 0, 4);
    addKnob(ParamIDs::sidechainLevelDb,     "Nivel SC",       loopSection, 0, 5);

    // Micro-loop, fila B
    addKnob(ParamIDs::glitchDegrade,        "Degradacion",    loopSection, 1, 0);
    addKnob(ParamIDs::glitchChaos,          "Caos",           loopSection, 1, 1);
    addKnob(ParamIDs::glitchReverseAmount,  "Reversa",        loopSection, 1, 2);
    addKnob(ParamIDs::glitchToneTilt,       "Tilt",           loopSection, 1, 3);
    addToggle(ParamIDs::selfGlitchEnabled,  "Auto-Glitch",    loopSection, 1, 4);
    addToggle(ParamIDs::loopKeep,           "Mantener",       loopSection, 1, 5);

    const int lowerWidth = (lfoCols + wetCols + loopCols) * cellWidth + 6 * panelPadding + 2 * sectionGap;
    const int innerWidth = juce::jmax(lowerWidth, synthCols * cellWidth + 2 * panelPadding);
    const int innerHeight = topBarHeight + sectionGap
                          + (headerHeight + synthRows * cellHeight + 2 * panelPadding) + sectionGap
                          + (headerHeight + effectRows * cellHeight + 2 * panelPadding) + sectionGap
                          + keyboardHeight;

    baseWidth = innerWidth + 2 * margin;
    baseHeight = innerHeight + 2 * margin;

    scaledComponents = { &keyboard, &presetLabel, &presetBox, &savePresetButton, &presetNameEditor };
    for (auto& label : sectionLabels)
        scaledComponents.push_back(&label);
    for (auto& waveEditor : waveEditors)
        scaledComponents.push_back(waveEditor.get());
    for (auto& control : controls)
    {
        scaledComponents.push_back(&control->label);
        if (control->slider != nullptr) scaledComponents.push_back(control->slider.get());
        if (control->combo != nullptr)  scaledComponents.push_back(control->combo.get());
        if (control->toggle != nullptr) scaledComponents.push_back(control->toggle.get());
    }

    setResizable(true, true);
    setResizeLimits((int) (baseWidth * 0.5), (int) (baseHeight * 0.5),
                    baseWidth * 2, baseHeight * 2);
    getConstrainer()->setFixedAspectRatio((double) baseWidth / (double) baseHeight);
    setSize(baseWidth, baseHeight);

    computerKeyNote.fill(-1);

    setWantsKeyboardFocus(true);
    grabKeyboardFocus();
}

GlitchSynthAudioProcessorEditor::~GlitchSynthAudioProcessorEditor()
{
    releaseAllComputerKeys();
    setLookAndFeel(nullptr);
}

GlitchSynthAudioProcessorEditor::Control& GlitchSynthAudioProcessorEditor::addControl(
    const juce::String& labelText, Section section, int row, int col)
{
    auto control = std::make_unique<Control>();
    control->section = section;
    control->row = row;
    control->col = col;

    control->label.setText(labelText.toUpperCase(), juce::dontSendNotification);
    control->label.setFont(SalirLookAndFeel::labelFont());
    control->label.setJustificationType(juce::Justification::centred);
    control->label.setColour(juce::Label::textColourId, Theme::textDim);
    control->label.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(control->label);

    controls.push_back(std::move(control));
    return *controls.back();
}

void GlitchSynthAudioProcessorEditor::addKnob(
    const juce::String& paramID, const juce::String& labelText, Section section, int row, int col)
{
    auto& control = addControl(labelText, section, row, col);

    control.slider = std::make_unique<juce::Slider>();
    control.slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    control.slider->setTextBoxStyle(juce::Slider::TextBoxBelow, true, cellWidth, SalirLookAndFeel::valueTextHeight);
    control.slider->setRotaryParameters(juce::degreesToRadians(225.0f), juce::degreesToRadians(495.0f), true);
    control.slider->setMouseDragSensitivity(220);
    addAndMakeVisible(*control.slider);

    control.sliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.apvts, paramID, *control.slider);

    // Double-click puts the knob back to its default.
    if (auto* param = processorRef.apvts.getParameter(paramID))
        control.slider->setDoubleClickReturnValue(true, param->convertFrom0to1(param->getDefaultValue()));
}

void GlitchSynthAudioProcessorEditor::addChoice(
    const juce::String& paramID, const juce::String& labelText, Section section, int row, int col)
{
    auto& control = addControl(labelText, section, row, col);

    control.combo = std::make_unique<juce::ComboBox>();
    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(processorRef.apvts.getParameter(paramID)))
        control.combo->addItemList(choice->choices, 1);
    control.combo->setWantsKeyboardFocus(false);
    addAndMakeVisible(*control.combo);

    control.comboAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processorRef.apvts, paramID, *control.combo);
}

void GlitchSynthAudioProcessorEditor::addToggle(
    const juce::String& paramID, const juce::String& labelText, Section section, int row, int col)
{
    auto& control = addControl(labelText, section, row, col);

    control.toggle = std::make_unique<juce::ToggleButton>();
    control.toggle->setWantsKeyboardFocus(false);
    addAndMakeVisible(*control.toggle);

    control.toggleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.apvts, paramID, *control.toggle);
}

void GlitchSynthAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Board-green backdrop with a gentle vignette toward the bottom.
    g.setGradientFill(juce::ColourGradient(Theme::background.brighter(0.08f), 0.0f, 0.0f,
                                           Theme::background.darker(0.35f), 0.0f, (float) getHeight(), false));
    g.fillAll();
    g.addTransform(juce::AffineTransform::scale(uiScale));

    for (int s = 0; s < numSections; ++s)
    {
        const auto panel = sectionPanels[(size_t) s].toFloat();
        SalirLookAndFeel::drawPanel(g, panel);

        // Hairline under the section title.
        const float lineY = panel.getY() + (float) (panelPadding + headerHeight) - 8.0f;
        g.setColour(Theme::panelEdge.withAlpha(0.6f));
        g.drawHorizontalLine((int) lineY, panel.getX() + (float) panelPadding, panel.getRight() - (float) panelPadding);
    }

    if (banner.isValid())
    {
        const auto area = bannerArea.toFloat();
        juce::Path clip;
        clip.addRoundedRectangle(area, 8.0f);

        juce::DropShadow(juce::Colours::black.withAlpha(0.5f), 12, { 0, 4 }).drawForPath(g, clip);

        juce::Graphics::ScopedSaveState saved(g);
        g.reduceClipRegion(clip);
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        g.drawImage(banner, area, juce::RectanglePlacement::stretchToFit);
    }

    // Frame around the keyboard so it sits in the layout like a panel.
    const auto keys = keyboard.getBounds().toFloat().expanded(1.0f);
    g.setColour(Theme::panelEdge);
    g.drawRoundedRectangle(keys, 3.0f, 1.0f);
}

void GlitchSynthAudioProcessorEditor::resized()
{
    if (baseWidth <= 0)
        return; // constructor hasn't finished setting up yet

    uiScale = juce::jmin((float) getWidth() / (float) baseWidth,
                         (float) getHeight() / (float) baseHeight);

    // Lay out in design-size coordinates; the transforms below do the scaling.
    auto bounds = juce::Rectangle<int>(0, 0, baseWidth, baseHeight).reduced(margin);

    auto keyboardArea = bounds.removeFromBottom(keyboardHeight);
    keyboard.setKeyWidth((float) keyboardArea.getWidth() / (float) numWhiteKeys());
    keyboard.setBounds(keyboardArea);
    bounds.removeFromBottom(sectionGap);

    auto topBar = bounds.removeFromTop(topBarHeight);

    // Banner keeps the image's own proportions, at the full top-bar height.
    const int bannerWidth = banner.isValid()
        ? juce::roundToInt((float) topBarHeight * (float) banner.getWidth() / (float) banner.getHeight())
        : 0;
    bannerArea = topBar.removeFromLeft(bannerWidth);

    auto presetRow = topBar.withSizeKeepingCentre(topBar.getWidth(), 30);
    savePresetButton.setBounds(presetRow.removeFromRight(84));
    presetRow.removeFromRight(8);
    presetBox.setBounds(presetRow.removeFromRight(240));
    presetNameEditor.setBounds(presetBox.getBounds());
    presetRow.removeFromRight(10);
    presetLabel.setBounds(presetRow.removeFromRight(70));
    bounds.removeFromTop(sectionGap);

    sectionPanels[synthSection] = bounds.removeFromTop(headerHeight + synthRows * cellHeight + 2 * panelPadding);
    bounds.removeFromTop(sectionGap);

    // LFO and the two effect channels side by side.
    auto lowerArea = bounds.removeFromTop(headerHeight + effectRows * cellHeight + 2 * panelPadding);
    sectionPanels[lfoSection] = lowerArea.removeFromLeft(lfoCols * cellWidth + 2 * panelPadding);
    lowerArea.removeFromLeft(sectionGap);
    sectionPanels[wetSection] = lowerArea.removeFromLeft(wetCols * cellWidth + 2 * panelPadding);
    lowerArea.removeFromLeft(sectionGap);
    sectionPanels[loopSection] = lowerArea;

    const std::array<int, numSections> sectionCols { synthCols, lfoCols, wetCols, loopCols };

    for (int s = 0; s < numSections; ++s)
    {
        auto panel = sectionPanels[(size_t) s].reduced(panelPadding);
        auto header = panel.removeFromTop(headerHeight);
        sectionLabels[(size_t) s].setBounds(header.removeFromTop(headerHeight - 12).withTrimmedLeft(2));

        sectionGrids[(size_t) s] = panel.withSizeKeepingCentre(sectionCols[(size_t) s] * cellWidth,
                                                               panel.getHeight());
    }

    // Every row is centred in its section, so short rows don't hug the left edge.
    std::map<std::pair<int, int>, int> rowLengths;
    for (const auto& control : controls)
    {
        auto& length = rowLengths[{ control->section, control->row }];
        length = juce::jmax(length, control->col + 1);
    }

    const auto rowOrigin = [&](int section, int row)
    {
        const auto& grid = sectionGrids[(size_t) section];
        const int rowWidth = rowLengths[{ section, row }] * cellWidth;
        return juce::Point<int>(grid.getX() + (grid.getWidth() - rowWidth) / 2, grid.getY() + row * cellHeight);
    };

    for (auto& control : controls)
    {
        const auto origin = rowOrigin(control->section, control->row);
        auto cell = juce::Rectangle<int>(origin.x + control->col * cellWidth, origin.y, cellWidth, cellHeight);

        control->label.setBounds(cell.removeFromTop(labelHeight));
        cell.removeFromTop(4);
        auto knobArea = cell.removeFromTop(knobSize + SalirLookAndFeel::valueTextHeight);

        if (control->slider != nullptr)
            control->slider->setBounds(knobArea);
        else if (control->combo != nullptr)
            control->combo->setBounds(knobArea.withSizeKeepingCentre(cellWidth - 16, 28)
                                              .withY(knobArea.getY() + (knobSize - 28) / 2));
        else if (control->toggle != nullptr)
            control->toggle->setBounds(knobArea.withSizeKeepingCentre(cellWidth - 16, 28)
                                               .withY(knobArea.getY() + (knobSize - 28) / 2));
    }

    for (int i = 0; i < GlitchSynthVoice::numOscillators; ++i)
    {
        const auto origin = rowOrigin(synthSection, i);
        waveEditors[(size_t) i]->setBounds(juce::Rectangle<int>(origin.x, origin.y,
                                                                waveEditorCols * cellWidth,
                                                                cellHeight).reduced(8, 6));
    }

    const auto scale = juce::AffineTransform::scale(uiScale);
    for (auto* component : scaledComponents)
        component->setTransform(scale);

    repaint();
}

void GlitchSynthAudioProcessorEditor::refreshPresetList()
{
    presetBox.clear(juce::dontSendNotification);

    const auto& presets = getFactoryPresets();
    for (int i = 0; i < (int) presets.size(); ++i)
        presetBox.addItem(presets[(size_t) i].name, i + 1);

    userPresetFiles = GlitchSynthAudioProcessor::getUserPresetFiles();
    if (!userPresetFiles.isEmpty())
    {
        presetBox.addSeparator();
        presetBox.addSectionHeading("MIS PRESETS");
        for (int i = 0; i < userPresetFiles.size(); ++i)
            presetBox.addItem(userPresetFiles[i].getFileNameWithoutExtension(), firstUserPresetId + i);
    }

    const int userIndex = userPresetFiles.indexOf(processorRef.currentUserPreset);
    presetBox.setSelectedId(userIndex >= 0 ? firstUserPresetId + userIndex
                                           : processorRef.getCurrentProgram() + 1,
                            juce::dontSendNotification);
}

void GlitchSynthAudioProcessorEditor::beginSavingPreset()
{
    // Start from the current user preset's name, so re-saving it is just Enter.
    presetNameEditor.setText(processorRef.currentUserPreset.getFileNameWithoutExtension(), false);
    presetNameEditor.selectAll();

    presetBox.setVisible(false);
    presetNameEditor.setVisible(true);
    presetNameEditor.grabKeyboardFocus();
    savePresetButton.setButtonText("OK");
}

void GlitchSynthAudioProcessorEditor::finishSavingPreset(bool save)
{
    if (!presetNameEditor.isVisible())
        return; // already finished (Enter also loses focus on the way out)

    presetNameEditor.setVisible(false);
    presetBox.setVisible(true);
    savePresetButton.setButtonText("Guardar");

    if (save && processorRef.saveUserPreset(presetNameEditor.getText()))
        refreshPresetList();

    // Back to the editor, so the computer keyboard plays notes again.
    grabKeyboardFocus();
}

int GlitchSynthAudioProcessorEditor::numWhiteKeys()
{
    int count = 0;
    for (int note = lowestKeyboardNote; note <= highestKeyboardNote; ++note)
        if (!juce::MidiMessage::isMidiNoteBlack(note))
            ++count;
    return count;
}

int GlitchSynthAudioProcessorEditor::computerKeyIndexFor(juce::juce_wchar c) const
{
    const auto upper = juce::CharacterFunctions::toUpperCase(c);

    for (int i = 0; i < numComputerKeyboardNotes; ++i)
        if ((juce::juce_wchar) computerKeyboardNoteChars[i] == upper)
            return i;

    return -1;
}

bool GlitchSynthAudioProcessorEditor::keyPressed(const juce::KeyPress& key)
{
    // Leave shortcuts (Ctrl+S etc.) to the host.
    if (key.getModifiers().isCommandDown() || key.getModifiers().isAltDown())
        return false;

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

    // Swallow note keys. Unconsumed keys are forwarded to the host, and FL Studio's
    // own typing keyboard then plays a second, differently-mapped note on top.
    return computerKeyIndexFor(c) >= 0;
}

bool GlitchSynthAudioProcessorEditor::keyStateChanged(bool /*isKeyDown*/)
{
    bool handledAny = false;

    for (int i = 0; i < numComputerKeyboardNotes; ++i)
    {
        const bool down = juce::KeyPress::isKeyCurrentlyDown((int) computerKeyboardNoteChars[i]);
        const bool held = computerKeyNote[(size_t) i] >= 0;

        if (down == held)
            continue;

        handledAny = true;

        if (down)
        {
            const int noteNumber = juce::jlimit(0, 127, computerKeyboardBaseNote + i);
            computerKeyNote[(size_t) i] = noteNumber;
            processorRef.keyboardState.noteOn(1, noteNumber, 1.0f);
        }
        else
        {
            releaseComputerKey(i);
        }
    }

    if (handledAny)
        startTimerHz(30);

    return handledAny;
}

void GlitchSynthAudioProcessorEditor::releaseComputerKey(int index)
{
    auto& note = computerKeyNote[(size_t) index];
    if (note < 0)
        return;

    processorRef.keyboardState.noteOff(1, note, 0.0f);
    note = -1;
}

void GlitchSynthAudioProcessorEditor::releaseAllComputerKeys()
{
    for (int i = 0; i < numComputerKeyboardNotes; ++i)
        releaseComputerKey(i);

    stopTimer();
}

void GlitchSynthAudioProcessorEditor::focusLost(FocusChangeType)
{
    // Focus moving to one of our own controls is fine: key events still bubble up here.
    if (hasKeyboardFocus(true))
        return;

    // Otherwise the key-up lands in another window and never reaches us.
    releaseAllComputerKeys();
}

void GlitchSynthAudioProcessorEditor::timerCallback()
{
    // Safety net for missed key-ups: poll the real key state and release anything
    // no longer held. Never starts notes, so typing in other windows stays silent.
    bool anyHeld = false;

    for (int i = 0; i < numComputerKeyboardNotes; ++i)
    {
        if (computerKeyNote[(size_t) i] < 0)
            continue;

        if (juce::KeyPress::isKeyCurrentlyDown((int) computerKeyboardNoteChars[i]))
            anyHeld = true;
        else
            releaseComputerKey(i);
    }

    if (!anyHeld)
        stopTimer();
}
