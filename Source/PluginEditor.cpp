#include "PluginEditor.h"

GlitchSynthAudioProcessorEditor::GlitchSynthAudioProcessorEditor(GlitchSynthAudioProcessor& p)
    : AudioProcessorEditor(&p),
      processorRef(p),
      genericEditor(p)
{
    addAndMakeVisible(genericEditor);
    setSize(500, 600);
    setResizable(true, true);
}

void GlitchSynthAudioProcessorEditor::resized()
{
    genericEditor.setBounds(getLocalBounds());
}
