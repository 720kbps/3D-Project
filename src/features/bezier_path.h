#pragma once
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/vec3.hpp>
DISABLE_WARNINGS_POP()
#include <framework/opengl_includes.h>
#include <vector>

class BezierPath {
public:
    BezierPath();
    BezierPath(const BezierPath&) = delete;
    BezierPath& operator=(const BezierPath&) = delete;
    ~BezierPath();

    // t goes from 0 to segmentCount(), then loops
    glm::vec3 position(float t) const;
    int segmentCount() const;
    void draw() const;

private:
    std::vector<glm::vec3> m_anchors;
    std::vector<glm::vec3> m_handles;

    GLuint m_vao { 0 };
    GLuint m_vbo { 0 };
    GLsizei m_lineVertexCount { 0 };
};
