#include "shadow_map.h"
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/gtc/matrix_transform.hpp>
DISABLE_WARNINGS_POP()
#include <cmath>

namespace {
// box around the scene, as seen from the sun
constexpr float sunDistance = 6.0f;
constexpr float boxHalfSize = 4.0f;
constexpr float nearPlane = 0.5f;
constexpr float farPlane = 12.0f;
}

ShadowMap::ShadowMap()
{
    // depth texture, as in exercise 4
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, size, size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // framebuffer that only writes depth into that texture
    glGenFramebuffers(1, &m_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_texture, 0);
    glDrawBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

ShadowMap::~ShadowMap()
{
    glDeleteFramebuffers(1, &m_framebuffer);
    glDeleteTextures(1, &m_texture);
}

glm::mat4 ShadowMap::sunMatrix(const glm::vec3& lightDirection)
{
    // camera placed towards the sun, looking at the scene centre
    const glm::vec3 direction = glm::normalize(lightDirection);
    const glm::vec3 up = std::abs(direction.y) > 0.99f ? glm::vec3(0.0f, 0.0f, -1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::mat4 view = glm::lookAt(sunDistance * direction, glm::vec3(0.0f), up);

    // sun rays are parallel, so the projection is orthographic
    const glm::mat4 projection = glm::ortho(-boxHalfSize, boxHalfSize, -boxHalfSize, boxHalfSize, nearPlane, farPlane);
    return projection * view;
}

void ShadowMap::bind() const
{
    // render into the depth texture, as in exercise 4
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    glViewport(0, 0, size, size);
    glEnable(GL_DEPTH_TEST);
    glClearDepth(1.0);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void ShadowMap::bindTexture(GLenum textureSlot) const
{
    glActiveTexture(textureSlot);
    glBindTexture(GL_TEXTURE_2D, m_texture);
}
