#pragma once
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/vec2.hpp>
DISABLE_WARNINGS_POP()
#include <framework/opengl_includes.h>

class Bloom {
public:
    Bloom();
    Bloom(const Bloom&) = delete;
    Bloom& operator=(const Bloom&) = delete;
    ~Bloom();

    void resize(const glm::ivec2& screenSize);
    void bindTarget(int index) const;
    void bindTexture(int index, GLenum textureSlot) const;

private:
    glm::ivec2 m_size { 0 };
    GLuint m_framebuffers[2] { 0, 0 };
    GLuint m_textures[2] { 0, 0 };
};
