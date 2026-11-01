#pragma once

#include "PluginProcessor.h"

class GlitchSynthAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit GlitchSynthAudioProcessorEditor(GlitchSynthAudioProcessor&);

    void resized() override;

private:
    GlitchSynthAudioProcessor& processorRef;
    juce::GenericAudioProcessorEditor genericEditor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlitchSynthAudioProcessorEditor)
};
