#include "arc_length.h"
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/geometric.hpp>
DISABLE_WARNINGS_POP()
#include <algorithm>
#include <cmath>

ArcLengthTable::ArcLengthTable(const BezierPath& path)
{
    // sample the curve densely
    const int samplesPerSegment = 200;
    const int sampleCount = path.segmentCount() * samplesPerSegment;
    m_step = 1.0f / static_cast<float>(samplesPerSegment);

    // add up the short straight pieces between samples
    m_lengths.push_back(0.0f);
    glm::vec3 previous = path.position(0.0f);
    for (int i = 1; i <= sampleCount; ++i) {
        const glm::vec3 current = path.position(static_cast<float>(i) * m_step);
        m_lengths.push_back(m_lengths.back() + glm::length(current - previous));
        previous = current;
    }
}

float ArcLengthTable::totalLength() const
{
    return m_lengths.back();
}

float ArcLengthTable::parameterAtDistance(float distance) const
{
    // wrap around the closed loop
    distance = std::fmod(distance, totalLength());
    if (distance < 0.0f)
        distance += totalLength();

    // binary search for the first sample further than the distance
    const auto next = std::upper_bound(m_lengths.begin(), m_lengths.end(), distance);
    const size_t i = std::clamp(static_cast<size_t>(next - m_lengths.begin()), size_t { 1 }, m_lengths.size() - 1);

    // interpolate between the two samples around it
    const float fraction = (distance - m_lengths[i - 1]) / (m_lengths[i] - m_lengths[i - 1]);
    return (static_cast<float>(i - 1) + fraction) * m_step;
}
