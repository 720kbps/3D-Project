#pragma once
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
DISABLE_WARNINGS_POP()
#include <framework/opengl_includes.h>

class ShadowMap {
public:
    ShadowMap();
    ShadowMap(const ShadowMap&) = delete;
    ShadowMap& operator=(const ShadowMap&) = delete;
    ~ShadowMap();

    // projection * view of the sun, shining from lightDirection
    static glm::mat4 sunMatrix(const glm::vec3& lightDirection);

    void bind() const;
    void bindTexture(GLenum textureSlot) const;

    static constexpr int size = 2048;

private:
    GLuint m_framebuffer { 0 };
    GLuint m_texture { 0 };
};
