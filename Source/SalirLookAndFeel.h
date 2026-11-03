#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

/** The plugin's hand-drawn look: machined knobs with a value ring, pill
    switches, soft drop-downs and quiet value read-outs, all in the chalkboard theme. */
class SalirLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SalirLookAndFeel();

    static juce::Font labelFont();
    static juce::Font valueFont();
    static juce::Font sectionFont();

    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider&) override;
    juce::Label* createSliderTextBox(juce::Slider&) override;
    void drawLabel(juce::Graphics&, juce::Label&) override;
    juce::Slider::SliderLayout getSliderLayout(juce::Slider&) override;

    void drawComboBox(juce::Graphics&, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    void positionComboBoxText(juce::ComboBox&, juce::Label&) override;

    juce::Font getPopupMenuFont() override;
    void drawPopupMenuBackground(juce::Graphics&, int width, int height) override;

    void drawToggleButton(juce::Graphics&, juce::ToggleButton&,
                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;

    /** A section panel: soft vertical gradient, hairline edge, faint top highlight. */
    static void drawPanel(juce::Graphics&, juce::Rectangle<float> area);

    static constexpr int valueTextHeight = 16;
};
