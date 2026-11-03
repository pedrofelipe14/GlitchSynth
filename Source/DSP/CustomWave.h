#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <atomic>
#include <cmath>

/** A user-shaped single-cycle waveform: a handful of control points joined by a
    smooth curve that loops around. The editor moves points on the message thread;
    the curve is rendered into a table the audio thread reads. Table entries are
    atomic, so a half-updated table just plays one slightly odd cycle. */
class CustomWave
{
public:
    static constexpr int numPoints = 8;
    static constexpr int tableSize = 256;

    struct Point
    {
        float x = 0.0f; // position in the cycle, 0..1 (point 0 is pinned to 0)
        float y = 0.0f; // level, -1..1
    };

    CustomWave() { reset(); }

    /** Evenly spaced points tracing a sine. Message thread only. */
    void reset()
    {
        for (int i = 0; i < numPoints; ++i)
        {
            const float x = (float) i / (float) numPoints;
            points[(size_t) i] = { x, std::sin(juce::MathConstants<float>::twoPi * x) };
        }
        renderTable();
    }

    Point getPoint(int index) const { return points[(size_t) index]; }

    /** Moves a point, keeping it between its neighbours. Message thread only. */
    void setPoint(int index, Point point)
    {
        constexpr float minGap = 0.02f;
        const float lowX  = index == 0 ? 0.0f : points[(size_t) index - 1].x + minGap;
        const float highX = index == numPoints - 1 ? 1.0f - minGap : points[(size_t) index + 1].x - minGap;

        point.x = index == 0 ? 0.0f : juce::jlimit(lowX, highX, point.x);
        point.y = juce::jlimit(-1.0f, 1.0f, point.y);
        points[(size_t) index] = point;
        renderTable();
    }

    /** Sets every point's level, leaving positions alone. Message thread only. */
    template <typename Fn>
    void setLevels(Fn levelAtPosition)
    {
        for (auto& point : points)
            point.y = juce::jlimit(-1.0f, 1.0f, levelAtPosition(point.x));
        renderTable();
    }

    /** Linearly interpolated table read, phase in [0, 1). Safe from the audio thread. */
    float sampleAt(float phase) const
    {
        const float position = phase * (float) tableSize;
        const int i0 = juce::jlimit(0, tableSize - 1, (int) position);
        const int i1 = (i0 + 1) % tableSize;
        const float frac = position - (float) i0;
        const float a = table[(size_t) i0].load(std::memory_order_relaxed);
        const float b = table[(size_t) i1].load(std::memory_order_relaxed);
        return a + frac * (b - a);
    }

    /** Bumped on every edit, so the UI knows when to repaint. */
    int getVersion() const { return version.load(std::memory_order_relaxed); }

    juce::String toString() const
    {
        juce::StringArray values;
        for (const auto& point : points)
            values.add(juce::String(point.x, 4) + ":" + juce::String(point.y, 4));
        return values.joinIntoString(",");
    }

    void fromString(const juce::String& text)
    {
        juce::StringArray values;
        values.addTokens(text, ",", {});
        if (values.size() != numPoints)
            return;

        for (int i = 0; i < numPoints; ++i)
            points[(size_t) i] = { values[i].upToFirstOccurrenceOf(":", false, false).getFloatValue(),
                                   values[i].fromFirstOccurrenceOf(":", false, false).getFloatValue() };
        renderTable();
    }

private:
    /** Catmull-Rom curve through the points, wrapping from the last back to the first. */
    float curveAt(float phase) const
    {
        int segment = numPoints - 1;
        for (int i = 0; i < numPoints - 1; ++i)
        {
            if (phase < points[(size_t) i + 1].x)
            {
                segment = i;
                break;
            }
        }

        const auto pointAt = [this](int i) { return points[(size_t) ((i + numPoints) % numPoints)]; };
        const float startX = points[(size_t) segment].x;
        const float endX = segment == numPoints - 1 ? 1.0f : points[(size_t) segment + 1].x;
        const float t = (phase - startX) / juce::jmax(1.0e-6f, endX - startX);

        const float y0 = pointAt(segment - 1).y;
        const float y1 = pointAt(segment).y;
        const float y2 = pointAt(segment + 1).y;
        const float y3 = pointAt(segment + 2).y;

        const float value = 0.5f * ((2.0f * y1)
                                    + (-y0 + y2) * t
                                    + (2.0f * y0 - 5.0f * y1 + 4.0f * y2 - y3) * t * t
                                    + (-y0 + 3.0f * y1 - 3.0f * y2 + y3) * t * t * t);
        return juce::jlimit(-1.0f, 1.0f, value);
    }

    void renderTable()
    {
        for (int i = 0; i < tableSize; ++i)
            table[(size_t) i].store(curveAt((float) i / (float) tableSize), std::memory_order_relaxed);
        version.fetch_add(1, std::memory_order_relaxed);
    }

    std::array<Point, numPoints> points;
    std::array<std::atomic<float>, tableSize> table;
    std::atomic<int> version { 0 };
};
