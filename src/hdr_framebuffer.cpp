#include "hdr_framebuffer.h"
#include <iostream>

namespace {
GLuint createTarget(GLint internalFormat, GLenum format, GLenum type, const glm::ivec2& size)
{
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, size.x, size.y, 0, format, type, nullptr);
    // Post passes read about one texel per pixel, so no mipmaps and no blending between texels.
    // CLAMP_TO_EDGE stops filters that sample neighbours (blur, edge detection) from wrapping around the screen.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return texture;
}
}

HdrFramebuffer::~HdrFramebuffer()
{
    release();
}

void HdrFramebuffer::resize(const glm::ivec2& size)
{
    // Size 0 happens while the window is minimised; keep the old target until it comes back.
    if (size == m_size || size.x <= 0 || size.y <= 0)
        return;
    release();
    m_size = size;

    m_color = createTarget(GL_RGBA16F, GL_RGBA, GL_FLOAT, size);
    m_normal = createTarget(GL_RGBA16F, GL_RGBA, GL_FLOAT, size);
    m_depth = createTarget(GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, size);

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_color, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_normal, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depth, 0);
    // Fragment shader output location 0 goes to the colour texture, location 1 to the normal texture.
    const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, drawBuffers);

    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "HdrFramebuffer is incomplete (status 0x" << std::hex << status << std::dec << ")" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void HdrFramebuffer::bindAndClear(const glm::vec3& clearColor) const
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_size.x, m_size.y);
    // glClear would give every colour attachment the same value; the normal buffer must be 0 (alpha 0 = "nothing here").
    const GLfloat color[] = { clearColor.r, clearColor.g, clearColor.b, 1.0f };
    const GLfloat zero[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    const GLfloat farthest = 1.0f;
    glClearBufferfv(GL_COLOR, 0, color);
    glClearBufferfv(GL_COLOR, 1, zero);
    glClearBufferfv(GL_DEPTH, 0, &farthest);
}

void HdrFramebuffer::release()
{
    glDeleteFramebuffers(1, &m_fbo);
    glDeleteTextures(1, &m_color);
    glDeleteTextures(1, &m_normal);
    glDeleteTextures(1, &m_depth);
    m_fbo = m_color = m_normal = m_depth = 0;
    m_size = glm::ivec2(0);
}

FullscreenTriangle::FullscreenTriangle()
{
    glGenVertexArrays(1, &m_vao);
}

FullscreenTriangle::~FullscreenTriangle()
{
    glDeleteVertexArrays(1, &m_vao);
}

void FullscreenTriangle::draw() const
{
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}
