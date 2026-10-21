#include "bezier_path.h"
#include <cmath>

BezierPath::BezierPath()
{
    // points the track passes through
    m_anchors = {
        { 2.40f, 1.00f, 0.00f },
        { 1.05f, 1.90f, -1.82f },
        { -1.25f, 0.70f, -2.17f },
        { -2.00f, 1.60f, 0.00f },
        { -1.20f, 0.50f, 2.08f },
        { 1.10f, 1.30f, 1.91f },
    };

    // direction and strength of the track at each anchor
    m_handles = {
        { 0.00f, 0.00f, -0.86f },
        { -0.65f, 0.00f, -0.38f },
        { -0.78f, 0.00f, 0.45f },
        { 0.00f, 0.00f, 0.72f },
        { 0.75f, 0.00f, 0.43f },
        { 0.69f, 0.00f, -0.40f },
    };

    // sample the curve into line points
    const int samplesPerSegment = 50;
    std::vector<glm::vec3> linePoints;
    for (int i = 0; i < segmentCount() * samplesPerSegment; ++i)
        linePoints.push_back(position(static_cast<float>(i) / samplesPerSegment));
    m_lineVertexCount = static_cast<GLsizei>(linePoints.size());

    // upload to GPU
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(linePoints.size() * sizeof(glm::vec3)), linePoints.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
}

BezierPath::~BezierPath()
{
    glDeleteBuffers(1, &m_vbo);
    glDeleteVertexArrays(1, &m_vao);
}

glm::vec3 BezierPath::position(float t) const
{
    // pick the segment and the local t inside it
    const int count = segmentCount();
    t = std::fmod(t, static_cast<float>(count));
    if (t < 0.0f)
        t += static_cast<float>(count);
    const int segment = static_cast<int>(t) % count;
    const float u = t - static_cast<float>(segment);

    // the 4 control points of this segment
    const int next = (segment + 1) % count;
    const glm::vec3 p0 = m_anchors[segment];
    const glm::vec3 p1 = m_anchors[segment] + m_handles[segment];
    const glm::vec3 p2 = m_anchors[next] - m_handles[next];
    const glm::vec3 p3 = m_anchors[next];

    // cubic Bezier
    const float v = 1.0f - u;
    return v * v * v * p0 + 3.0f * v * v * u * p1 + 3.0f * v * u * u * p2 + u * u * u * p3;
}

int BezierPath::segmentCount() const
{
    return static_cast<int>(m_anchors.size());
}

void BezierPath::draw() const
{
    glBindVertexArray(m_vao);
    glDrawArrays(GL_LINE_LOOP, 0, m_lineVertexCount);
}
