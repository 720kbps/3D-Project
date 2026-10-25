#include "robot_arm.h"
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/gtc/matrix_transform.hpp>
DISABLE_WARNINGS_POP()
#include <cmath>

namespace {
// part sizes (x, length along y, z)
constexpr glm::vec3 pedestalSize { 0.5f, 0.08f, 0.5f };
constexpr glm::vec3 turretSize { 0.3f, 0.12f, 0.3f };
constexpr glm::vec3 upperArmSize { 0.1f, 0.45f, 0.1f };
constexpr glm::vec3 forearmSize { 0.08f, 0.38f, 0.08f };
constexpr glm::vec3 handSize { 0.18f, 0.06f, 0.08f };
constexpr glm::vec3 fingerSize { 0.03f, 0.12f, 0.05f };

constexpr glm::vec3 xAxis { 1.0f, 0.0f, 0.0f };
constexpr glm::vec3 yAxis { 0.0f, 1.0f, 0.0f };
constexpr glm::vec3 zAxis { 0.0f, 0.0f, 1.0f };

// unit mesh scaled to size, from the frame's origin along its +y
glm::mat4 partOnFrame(const glm::mat4& frame, const glm::vec3& size)
{
    return frame
        * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.5f * size.y, 0.0f))
        * glm::scale(glm::mat4(1.0f), size);
}

// unit sphere scaled to a diameter, centred on the frame's origin
glm::mat4 jointOnFrame(const glm::mat4& frame, float diameter)
{
    return frame * glm::scale(glm::mat4(1.0f), glm::vec3(diameter));
}
}

void RobotArm::update(float time)
{
    // idle motion: a sine wave that travels from the shoulder to the wrist
    if (animate) {
        const float wave = 0.8f * time;
        const float phaseOffset = 0.6f;
        turretAngle = 0.6f * std::sin(0.3f * time);
        shoulderAngle = 0.4f + 0.25f * std::sin(wave);
        elbowAngle = 0.9f + 0.35f * std::sin(wave - phaseOffset);
        wristAngle = 0.5f + 0.4f * std::sin(wave - 2.0f * phaseOffset);
        gripAngle = 0.25f + 0.25f * std::sin(2.0f * wave);
    }

    // pedestal: fixed on the ground
    const glm::mat4 baseFrame = glm::translate(glm::mat4(1.0f), position);
    m_partMatrices[Pedestal] = partOnFrame(baseFrame, pedestalSize);

    // turret: on top of the pedestal, turns around y
    const glm::mat4 turretFrame = baseFrame
        * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, pedestalSize.y, 0.0f))
        * glm::rotate(glm::mat4(1.0f), turretAngle, yAxis);
    m_partMatrices[Turret] = partOnFrame(turretFrame, turretSize);

    // shoulder: on top of the turret, tilts around x
    const glm::mat4 shoulderFrame = turretFrame
        * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, turretSize.y, 0.0f))
        * glm::rotate(glm::mat4(1.0f), shoulderAngle, xAxis);
    m_partMatrices[Shoulder] = jointOnFrame(shoulderFrame, 0.13f);
    m_partMatrices[UpperArm] = partOnFrame(shoulderFrame, upperArmSize);

    // elbow: at the end of the upper arm
    const glm::mat4 elbowFrame = shoulderFrame
        * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, upperArmSize.y, 0.0f))
        * glm::rotate(glm::mat4(1.0f), elbowAngle, xAxis);
    m_partMatrices[Elbow] = jointOnFrame(elbowFrame, 0.11f);
    m_partMatrices[Forearm] = partOnFrame(elbowFrame, forearmSize);

    // wrist: at the end of the forearm
    const glm::mat4 wristFrame = elbowFrame
        * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, forearmSize.y, 0.0f))
        * glm::rotate(glm::mat4(1.0f), wristAngle, xAxis);
    m_partMatrices[Wrist] = jointOnFrame(wristFrame, 0.09f);
    m_partMatrices[Hand] = partOnFrame(wristFrame, handSize);

    // fingers: at both ends of the hand, open outwards around z
    const float fingerOffset = 0.5f * (handSize.x - fingerSize.x);
    const glm::mat4 leftFingerFrame = wristFrame
        * glm::translate(glm::mat4(1.0f), glm::vec3(-fingerOffset, handSize.y, 0.0f))
        * glm::rotate(glm::mat4(1.0f), gripAngle, zAxis);
    m_partMatrices[LeftFinger] = partOnFrame(leftFingerFrame, fingerSize);

    const glm::mat4 rightFingerFrame = wristFrame
        * glm::translate(glm::mat4(1.0f), glm::vec3(fingerOffset, handSize.y, 0.0f))
        * glm::rotate(glm::mat4(1.0f), -gripAngle, zAxis);
    m_partMatrices[RightFinger] = partOnFrame(rightFingerFrame, fingerSize);
}

const std::array<glm::mat4, RobotArm::PartCount>& RobotArm::partMatrices() const
{
    return m_partMatrices;
}
