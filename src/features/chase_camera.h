#pragma once
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
DISABLE_WARNINGS_POP()

class ChaseCamera {
public:
    glm::mat4 viewMatrix(const glm::vec3& cartPosition) const;

    glm::vec3 target { 0.0f, 0.35f, 0.0f };
    float distanceBehind { 0.6f };
    float heightAbove { 0.25f };
};
