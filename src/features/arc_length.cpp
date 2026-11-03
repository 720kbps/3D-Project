#include "arc_length.h"
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
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

void ArcLengthTable::drawMarkers(const BezierPath& path, int count, const Shader& shader, const glm::mat4& viewProjection, const std::vector<GPUMesh>& sphereMesh) const
{
    // flat colour, no texture
    glUniform1i(shader.getUniformLocation("unlit"), true);
    glUniform1i(shader.getUniformLocation("hasAlbedoMap"), false);

    const glm::vec3 equalTColor { 1.0f, 0.35f, 0.1f };
    const glm::vec3 equalDistanceColor { 0.1f, 0.8f, 1.0f };
    for (int k = 0; k < count; ++k) {
        const float fraction = static_cast<float>(k) / static_cast<float>(count);

        // equal steps in t, and equal steps along the curve
        const float equalT = fraction * static_cast<float>(path.segmentCount());
        const float equalDistance = parameterAtDistance(fraction * totalLength());

        for (int set = 0; set < 2; ++set) {
            const glm::vec3 position = path.position(set == 0 ? equalT : equalDistance);
            const float size = set == 0 ? 0.07f : 0.045f;
            const glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), position), glm::vec3(size));
            const glm::mat4 mvpMatrix = viewProjection * model;
            glUniformMatrix4fv(shader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
            glUniformMatrix4fv(shader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(shader.getUniformLocation("baseColor"), 1, glm::value_ptr(set == 0 ? equalTColor : equalDistanceColor));
            for (const GPUMesh& mesh : sphereMesh)
                mesh.draw();
        }
    }
}
