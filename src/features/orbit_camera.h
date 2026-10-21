#pragma once
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
DISABLE_WARNINGS_POP()
#include <framework/window.h>

class OrbitCamera {
public:
    void updateInput(const Window& window, bool enabled);
    void zoom(float scrollAmount);
    glm::mat4 viewMatrix() const;

    glm::vec3 target { 0.0f };
    float distance { 2.5f };
    float yaw { 0.0f };
    float pitch { 0.35f };

private:
    glm::vec2 m_prevCursorPos { 0.0f };
};
