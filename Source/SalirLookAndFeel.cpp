#include "SalirLookAndFeel.h"
#include "Theme.h"

namespace
{
    // Knob body: neutral graphite with a hint of the board green, lit from above.
    const juce::Colour knobRimTop    { 0xff46544b };
    const juce::Colour knobRimBottom { 0xff0b120e };
    const juce::Colour knobFaceTop   { 0xff2b3731 };
    const juce::Colour knobFaceBottom{ 0xff151c18 };
    const juce::Colour ringTrack     { 0xff0a1610 };
}

SalirLookAndFeel::SalirLookAndFeel()
{
    setDefaultSansSerifTypefaceName("Segoe UI");

    setColour(juce::ResizableWindow::backgroundColourId, Theme::background);
    setColour(juce::Label::textColourId, Theme::text);

    setColour(juce::Slider::textBoxTextColourId, Theme::textDim);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxHighlightColourId, Theme::accentDim);

    setColour(juce::ComboBox::textColourId, Theme::text);
    setColour(juce::ComboBox::arrowColourId, Theme::accent);
    setColour(juce::PopupMenu::backgroundColourId, Theme::panel);
    setColour(juce::PopupMenu::textColourId, Theme::text);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, Theme::accentDim);
    setColour(juce::PopupMenu::highlightedTextColourId, Theme::text);

    setColour(juce::TextButton::textColourOffId, Theme::textDim);
    setColour(juce::TextButton::textColourOnId, Theme::text);

    setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, Theme::accent.withAlpha(0.75f));
    setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, Theme::accent.withAlpha(0.25f));
    setColour(juce::MidiKeyboardComponent::whiteNoteColourId, Theme::text);
    setColour(juce::MidiKeyboardComponent::blackNoteColourId, Theme::background);
    setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, Theme::panelEdge);
    setColour(juce::MidiKeyboardComponent::shadowColourId, juce::Colours::black.withAlpha(0.35f));
    setColour(juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, Theme::panel);
    setColour(juce::MidiKeyboardComponent::upDownButtonArrowColourId, Theme::textDim);
}

juce::Font SalirLookAndFeel::labelFont()
{
    return juce::Font(juce::FontOptions(10.5f, juce::Font::bold)).withExtraKerningFactor(0.12f);
}

juce::Font SalirLookAndFeel::valueFont()
{
    return juce::Font(juce::FontOptions(11.5f));
}

juce::Font SalirLookAndFeel::sectionFont()
{
    return juce::Font(juce::FontOptions(12.5f, juce::Font::bold)).withExtraKerningFactor(0.22f);
}

//==============================================================================
void SalirLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float startAngle, float endAngle,
                                        juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float>((float) x, (float) y, (float) width, (float) height);
    const float size = juce::jmin(bounds.getWidth(), bounds.getHeight());
    const auto area = bounds.withSizeKeepingCentre(size, size).reduced(2.0f);
    const auto centre = area.getCentre();
    const bool active = slider.isMouseOverOrDragging() && slider.isEnabled();

    const float ringRadius = area.getWidth() * 0.5f - 2.0f;
    const float ringThickness = 2.6f;
    const float angle = startAngle + sliderPos * (endAngle - startAngle);

    // Track ring.
    juce::Path track;
    track.addCentredArc(centre.x, centre.y, ringRadius, ringRadius, 0.0f, startAngle, endAngle, true);
    g.setColour(ringTrack);
    g.strokePath(track, juce::PathStrokeType(ringThickness, juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded));

    // Value ring: from the start, or from zero for bipolar controls (fine tune, env amount...).
    const bool bipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
    const float fromAngle = bipolar
        ? startAngle + (float) slider.valueToProportionOfLength(0.0) * (endAngle - startAngle)
        : startAngle;

    if (std::abs(angle - fromAngle) > 0.001f)
    {
        juce::Path valueArc;
        valueArc.addCentredArc(centre.x, centre.y, ringRadius, ringRadius, 0.0f,
                               juce::jmin(fromAngle, angle), juce::jmax(fromAngle, angle), true);

        g.setColour(Theme::accent.withAlpha(active ? 0.22f : 0.13f));
        g.strokePath(valueArc, juce::PathStrokeType(ringThickness + 4.0f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
        g.setColour(Theme::accent);
        g.strokePath(valueArc, juce::PathStrokeType(ringThickness, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
    }

    // Knob body with a soft drop shadow.
    const float bodyRadius = ringRadius - 7.0f;
    const auto body = juce::Rectangle<float>(bodyRadius * 2.0f, bodyRadius * 2.0f).withCentre(centre);

    juce::Path bodyPath;
    bodyPath.addEllipse(body);
    juce::DropShadow(juce::Colours::black.withAlpha(0.6f), 9, { 0, 4 }).drawForPath(g, bodyPath);

    // Rim: bright where the light hits the top edge, falling into shadow below.
    g.setGradientFill(juce::ColourGradient(knobRimTop, centre.x, body.getY(),
                                           knobRimBottom, centre.x, body.getBottom(), false));
    g.fillEllipse(body);

    // Face.
    const auto face = body.reduced(2.0f);
    g.setGradientFill(juce::ColourGradient(knobFaceTop, centre.x, face.getY(),
                                           knobFaceBottom, centre.x, face.getBottom(), false));
    g.fillEllipse(face);

    // Soft specular sheen near the top.
    const juce::Point<float> sheenCentre { centre.x, face.getY() + face.getHeight() * 0.28f };
    g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(active ? 0.11f : 0.07f), sheenCentre,
                                           juce::Colours::transparentWhite,
                                           sheenCentre.translated(0.0f, face.getHeight() * 0.55f), true));
    g.fillEllipse(face);

    // Pointer.
    juce::Path pointer;
    pointer.startNewSubPath(centre.getPointOnCircumference(bodyRadius * 0.30f, angle));
    pointer.lineTo(centre.getPointOnCircumference(bodyRadius * 0.80f, angle));
    g.setColour(Theme::text);
    g.strokePath(pointer, juce::PathStrokeType(2.4f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
}

juce::Label* SalirLookAndFeel::createSliderTextBox(juce::Slider& slider)
{
    auto* label = LookAndFeel_V4::createSliderTextBox(slider);
    label->setFont(valueFont());
    label->setJustificationType(juce::Justification::centred);
    label->setColour(juce::Label::textColourId, Theme::textDim);
    label->setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    label->setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
    return label;
}

void SalirLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    // Text only: no boxes or outlines around labels and value read-outs.
    g.fillAll(label.findColour(juce::Label::backgroundColourId));

    if (label.isBeingEdited())
        return;

    const float alpha = label.isEnabled() ? 1.0f : 0.5f;
    const auto font = getLabelFont(label);
    const auto area = getLabelBorderSize(label).subtractedFrom(label.getLocalBounds());

    g.setColour(label.findColour(juce::Label::textColourId).withMultipliedAlpha(alpha));
    g.setFont(font);
    g.drawFittedText(label.getText(), area, label.getJustificationType(),
                     juce::jmax(1, (int) ((float) area.getHeight() / font.getHeight())),
                     juce::jmax(0.75f, label.getMinimumHorizontalScale()));
}

juce::Slider::SliderLayout SalirLookAndFeel::getSliderLayout(juce::Slider& slider)
{
    juce::Slider::SliderLayout layout;
    auto bounds = slider.getLocalBounds();
    layout.textBoxBounds = bounds.removeFromBottom(valueTextHeight);
    layout.sliderBounds = bounds;
    return layout;
}

//==============================================================================
void SalirLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                                    int, int, int, int, juce::ComboBox& box)
{
    const auto bounds = juce::Rectangle<float>(0.0f, 0.0f, (float) width, (float) height).reduced(0.5f);
    const bool active = box.isMouseOver(true) || box.isPopupActive();

    g.setGradientFill(juce::ColourGradient(Theme::panel.brighter(active ? 0.18f : 0.10f), 0.0f, 0.0f,
                                           Theme::background.brighter(0.04f), 0.0f, (float) height, false));
    g.fillRoundedRectangle(bounds, 5.0f);

    g.setColour(active ? Theme::accent.withAlpha(0.55f) : Theme::panelEdge);
    g.drawRoundedRectangle(bounds, 5.0f, 1.0f);

    // Chevron.
    const float cx = (float) width - 13.0f;
    const float cy = (float) height * 0.5f;
    juce::Path chevron;
    chevron.startNewSubPath(cx - 4.0f, cy - 2.0f);
    chevron.lineTo(cx, cy + 2.0f);
    chevron.lineTo(cx + 4.0f, cy - 2.0f);
    g.setColour(Theme::accent);
    g.strokePath(chevron, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
}

juce::Font SalirLookAndFeel::getComboBoxFont(juce::ComboBox&)
{
    return juce::Font(juce::FontOptions(12.5f));
}

void SalirLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label)
{
    label.setBounds(8, 0, box.getWidth() - 28, box.getHeight());
    label.setFont(getComboBoxFont(box));
}

juce::Font SalirLookAndFeel::getPopupMenuFont()
{
    return juce::Font(juce::FontOptions(13.0f));
}

void SalirLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    g.fillAll(Theme::panel);
    g.setColour(Theme::panelEdge);
    g.drawRect(0, 0, width, height);
}

//==============================================================================
void SalirLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                        bool shouldDrawButtonAsHighlighted, bool)
{
    // A pill switch, centred when there's no caption (the grid label sits above it).
    const bool on = button.getToggleState();
    const auto bounds = button.getLocalBounds().toFloat();
    const juce::Rectangle<float> pillSize { 36.0f, 19.0f };

    auto pill = button.getButtonText().isEmpty()
        ? pillSize.withCentre(bounds.getCentre())
        : pillSize.withPosition(bounds.getX() + 2.0f, bounds.getCentreY() - pillSize.getHeight() * 0.5f);

    const float radius = pill.getHeight() * 0.5f;
    g.setColour(on ? Theme::accent.withAlpha(shouldDrawButtonAsHighlighted ? 0.95f : 0.85f)
                   : ringTrack);
    g.fillRoundedRectangle(pill, radius);
    g.setColour(on ? Theme::accent : Theme::panelEdge.brighter(shouldDrawButtonAsHighlighted ? 0.3f : 0.0f));
    g.drawRoundedRectangle(pill.reduced(0.5f), radius, 1.0f);

    const float thumbSize = pill.getHeight() - 5.0f;
    const float thumbX = on ? pill.getRight() - thumbSize - 2.5f : pill.getX() + 2.5f;
    const auto thumb = juce::Rectangle<float>(thumbX, pill.getY() + 2.5f, thumbSize, thumbSize);

    juce::Path thumbPath;
    thumbPath.addEllipse(thumb);
    juce::DropShadow(juce::Colours::black.withAlpha(0.45f), 4, { 0, 1 }).drawForPath(g, thumbPath);
    g.setColour(on ? Theme::background : Theme::textDim);
    g.fillEllipse(thumb);

    if (button.getButtonText().isNotEmpty())
    {
        g.setColour(Theme::text);
        g.setFont(labelFont());
        g.drawText(button.getButtonText(), bounds.withTrimmedLeft(pill.getWidth() + 10.0f),
                   juce::Justification::centredLeft);
    }
}

void SalirLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                            bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    auto fill = Theme::panel;
    if (shouldDrawButtonAsDown)             fill = fill.darker(0.2f);
    else if (shouldDrawButtonAsHighlighted) fill = fill.brighter(0.2f);

    g.setColour(fill);
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(shouldDrawButtonAsHighlighted ? Theme::accent.withAlpha(0.5f) : Theme::panelEdge);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
}

juce::Font SalirLookAndFeel::getTextButtonFont(juce::TextButton&, int)
{
    return labelFont();
}

//==============================================================================
void SalirLookAndFeel::drawPanel(juce::Graphics& g, juce::Rectangle<float> area)
{
    juce::Path shape;
    shape.addRoundedRectangle(area, 10.0f);
    juce::DropShadow(juce::Colours::black.withAlpha(0.45f), 14, { 0, 5 }).drawForPath(g, shape);

    g.setGradientFill(juce::ColourGradient(Theme::panel.brighter(0.05f), 0.0f, area.getY(),
                                           Theme::panel.darker(0.25f), 0.0f, area.getBottom(), false));
    g.fillPath(shape);

    g.setColour(Theme::panelEdge.withAlpha(0.8f));
    g.strokePath(shape, juce::PathStrokeType(1.0f));

    // Faint highlight along the top edge, like light catching a bevel.
    g.setColour(juce::Colours::white.withAlpha(0.06f));
    g.drawHorizontalLine((int) area.getY() + 1, area.getX() + 10.0f, area.getRight() - 10.0f);
}
