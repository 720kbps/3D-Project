#pragma once
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/vec2.hpp>
DISABLE_WARNINGS_POP()
#include <framework/opengl_includes.h>

class HdrFramebuffer {
public:
    HdrFramebuffer();
    HdrFramebuffer(const HdrFramebuffer&) = delete;
    HdrFramebuffer& operator=(const HdrFramebuffer&) = delete;
    ~HdrFramebuffer();

    void resize(const glm::ivec2& size);
    void bind() const;
    void bindColorTexture(GLenum textureSlot) const;
    void drawFullscreenTriangle() const;

private:
    glm::ivec2 m_size { 0 };
    GLuint m_framebuffer { 0 };
    GLuint m_colorTexture { 0 };
    GLuint m_depthTexture { 0 };
    GLuint m_emptyVao { 0 };
};
