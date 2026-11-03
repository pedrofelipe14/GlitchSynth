#include "WaveEditor.h"
#include "Theme.h"

WaveEditor::WaveEditor(GlitchSynthAudioProcessor& processor, int oscIndex,
                       const juce::String& waveformParamID, const juce::String& titleText)
    : wave(processor.customWaves[(size_t) oscIndex]),
      waveformParam(processor.apvts.getParameter(waveformParamID)),
      waveformValue(processor.apvts.getRawParameterValue(waveformParamID)),
      title(titleText)
{
    resetButton.setWantsKeyboardFocus(false); // keep note keys going to the editor
    resetButton.setColour(juce::TextButton::buttonColourId, Theme::panel);
    resetButton.setColour(juce::TextButton::textColourOffId, Theme::textDim);
    resetButton.onClick = [this]
    {
        switchToCustom();
        wave.reset();
        repaint();
    };
    addAndMakeVisible(resetButton);

    startTimerHz(15);
}

OscWaveform WaveEditor::currentWaveform() const
{
    return (OscWaveform) (int) waveformValue->load();
}

float WaveEditor::shapeAt(float phase) const
{
    return isCustom() ? wave.sampleAt(phase) : naiveWaveformSample(currentWaveform(), phase);
}

juce::Rectangle<float> WaveEditor::plotArea() const
{
    return getLocalBounds().toFloat().reduced(10.0f, 8.0f).withTrimmedTop(14.0f);
}

juce::Point<float> WaveEditor::toScreen(float x, float y) const
{
    const auto plot = plotArea();
    return { plot.getX() + x * plot.getWidth(), plot.getCentreY() - y * plot.getHeight() * 0.5f };
}

juce::Point<float> WaveEditor::pointOnScreen(int index) const
{
    const auto point = wave.getPoint(index);
    const float y = isCustom() ? point.y : naiveWaveformSample(currentWaveform(), point.x);
    return toScreen(point.x, y);
}

int WaveEditor::pointIndexNear(juce::Point<float> position) const
{
    int nearest = -1;
    float nearestDistance = grabRadius;

    for (int i = 0; i < CustomWave::numPoints; ++i)
    {
        const float distance = position.getDistanceFrom(pointOnScreen(i));
        if (distance < nearestDistance)
        {
            nearestDistance = distance;
            nearest = i;
        }
    }

    return nearest;
}

void WaveEditor::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setColour(Theme::background);
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(Theme::panelEdge);
    g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);

    g.setColour(Theme::accent);
    g.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
    g.drawText(title, bounds.reduced(8.0f, 4.0f).removeFromTop(14.0f), juce::Justification::centredLeft);

    const auto plot = plotArea();
    g.setColour(Theme::panelEdge);
    g.drawHorizontalLine((int) plot.getCentreY(), plot.getX(), plot.getRight());

    juce::Path path;
    const int steps = juce::jmax(2, (int) plot.getWidth());
    for (int i = 0; i <= steps; ++i)
    {
        const float x = (float) i / (float) steps;
        const auto screen = toScreen(x, shapeAt(juce::jmin(x, 0.9999f)));

        if (i == 0)
            path.startNewSubPath(screen);
        else
            path.lineTo(screen);
    }

    const bool custom = isCustom();
    g.setColour(custom ? Theme::accent : Theme::text.withAlpha(0.8f));
    g.strokePath(path, juce::PathStrokeType(1.6f));

    for (int i = 0; i < CustomWave::numPoints; ++i)
    {
        const bool active = i == hoveredPoint || i == draggedPoint;
        const float radius = active ? pointRadius + 1.5f : pointRadius;

        g.setColour(custom ? Theme::text : Theme::textDim.withAlpha(active ? 0.9f : 0.35f));
        g.fillEllipse(juce::Rectangle<float>(radius * 2.0f, radius * 2.0f).withCentre(pointOnScreen(i)));
    }

    if (!custom)
    {
        g.setColour(Theme::textDim.withAlpha(0.6f));
        g.setFont(juce::Font(juce::FontOptions(11.0f)));
        g.drawText("arrastra un punto para editar", plot.reduced(2.0f), juce::Justification::bottomRight);
    }
}

void WaveEditor::resized()
{
    resetButton.setBounds(getLocalBounds().removeFromTop(20).removeFromRight(52).reduced(2));
}

void WaveEditor::switchToCustom()
{
    const auto waveform = currentWaveform();
    if (waveform == OscWaveform::custom)
        return;

    wave.setLevels([waveform](float x) { return naiveWaveformSample(waveform, x); });

    if (waveformParam != nullptr)
    {
        waveformParam->beginChangeGesture();
        waveformParam->setValueNotifyingHost(
            waveformParam->convertTo0to1((float) OscWaveform::custom));
        waveformParam->endChangeGesture();
    }
}

void WaveEditor::setHoveredPoint(int index)
{
    if (index == hoveredPoint)
        return;

    hoveredPoint = index;
    setMouseCursor(index >= 0 ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::NormalCursor);
    repaint();
}

void WaveEditor::mouseMove(const juce::MouseEvent& e)
{
    setHoveredPoint(pointIndexNear(e.position));
}

void WaveEditor::mouseExit(const juce::MouseEvent&)
{
    setHoveredPoint(-1);
}

void WaveEditor::mouseDown(const juce::MouseEvent& e)
{
    draggedPoint = pointIndexNear(e.position);
    if (draggedPoint >= 0)
        switchToCustom();
}

void WaveEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (draggedPoint < 0)
        return;

    const auto plot = plotArea();
    wave.setPoint(draggedPoint, { (e.position.x - plot.getX()) / plot.getWidth(),
                                  (plot.getCentreY() - e.position.y) / (plot.getHeight() * 0.5f) });
    repaint();
}

void WaveEditor::mouseUp(const juce::MouseEvent& e)
{
    draggedPoint = -1;
    setHoveredPoint(pointIndexNear(e.position));
    repaint();
}

void WaveEditor::timerCallback()
{
    // Repaint when the shape changes from outside (combo box, preset, host automation).
    const int version = wave.getVersion();
    const int waveform = (int) currentWaveform();

    if (version != lastSeenVersion || waveform != lastSeenWaveform)
    {
        lastSeenVersion = version;
        lastSeenWaveform = waveform;
        repaint();
    }
}
