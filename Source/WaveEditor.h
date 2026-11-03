#pragma once

#include "PluginProcessor.h"

/** Shows one oscillator's waveform with a few draggable points that shape it.
    Dragging a point while a preset shape (saw, square...) is playing starts from
    that shape and switches the oscillator to "Propia". */
class WaveEditor : public juce::Component,
                   private juce::Timer
{
public:
    WaveEditor(GlitchSynthAudioProcessor& processor, int oscIndex,
               const juce::String& waveformParamID, const juce::String& title);

    void paint(juce::Graphics&) override;
    void resized() override;

    void mouseMove(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;

private:
    void timerCallback() override;

    OscWaveform currentWaveform() const;
    bool isCustom() const { return currentWaveform() == OscWaveform::custom; }
    float shapeAt(float phase) const;

    /** Where a point is drawn: on the custom curve, or on the preset shape it would start from. */
    juce::Point<float> pointOnScreen(int index) const;

    /** If a preset shape is playing, copies it onto the points and selects "Propia". */
    void switchToCustom();

    juce::Rectangle<float> plotArea() const;
    juce::Point<float> toScreen(float x, float y) const;
    int pointIndexNear(juce::Point<float> position) const;
    void setHoveredPoint(int index);

    CustomWave& wave;
    juce::RangedAudioParameter* waveformParam;
    std::atomic<float>* waveformValue;
    juce::String title;

    juce::TextButton resetButton { "Reset" };

    int hoveredPoint = -1;
    int draggedPoint = -1;

    int lastSeenVersion = -1;
    int lastSeenWaveform = -1;

    static constexpr float pointRadius = 4.5f;
    static constexpr float grabRadius = 11.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveEditor)
};
