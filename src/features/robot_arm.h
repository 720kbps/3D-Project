#pragma once
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
DISABLE_WARNINGS_POP()
#include <array>
#include <cstddef>

class RobotArm {
public:
    // drawn parts, parents before children
    enum Part : size_t {
        Pedestal,
        Turret,
        Shoulder,
        UpperArm,
        Elbow,
        Forearm,
        Wrist,
        Hand,
        LeftFinger,
        RightFinger,
        PartCount
    };

    void update(float time);
    const std::array<glm::mat4, PartCount>& partMatrices() const;

    glm::vec3 position { 0.0f, 0.0f, -1.6f };
    bool animate { true };

    // joint angles in radians
    float turretAngle { 0.0f };
    float shoulderAngle { 0.4f };
    float elbowAngle { 0.9f };
    float wristAngle { 0.5f };
    float gripAngle { 0.3f };

private:
    std::array<glm::mat4, PartCount> m_partMatrices {};
};
