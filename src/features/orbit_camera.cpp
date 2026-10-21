#include "orbit_camera.h"
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
DISABLE_WARNINGS_POP()
#include <cmath>

void OrbitCamera::updateInput(const Window& window, bool enabled)
{
    const glm::vec2 cursorPos = window.getCursorPos();
    const glm::vec2 delta = cursorPos - m_prevCursorPos;
    m_prevCursorPos = cursorPos;

    // drag with the left mouse button to rotate
    if (enabled && window.isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
        yaw -= delta.x * 0.005f;
        pitch += delta.y * 0.005f;
        pitch = glm::clamp(pitch, -1.5f, 1.5f);
    }
}

void OrbitCamera::zoom(float scrollAmount)
{
    distance = glm::clamp(distance - scrollAmount * 0.2f, 0.5f, 10.0f);
}

glm::mat4 OrbitCamera::viewMatrix() const
{
    // position on a sphere around the target
    const glm::vec3 offset {
        std::cos(pitch) * std::sin(yaw),
        std::sin(pitch),
        std::cos(pitch) * std::cos(yaw)
    };
    return glm::lookAt(target + distance * offset, target, glm::vec3(0.0f, 1.0f, 0.0f));
}
