#include "hdr_framebuffer.h"

static GLuint createTexture()
{
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return texture;
}

HdrFramebuffer::HdrFramebuffer()
{
    // create textures
    m_colorTexture = createTexture();
    m_depthTexture = createTexture();
    m_normalTexture = createTexture();

    // attach them to a framebuffer
    glGenFramebuffers(1, &m_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTexture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_normalTexture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTexture, 0);

    // shaders write color to output 0 and normals to output 1
    const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, drawBuffers);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // empty VAO for the fullscreen triangle
    glGenVertexArrays(1, &m_emptyVao);
}

HdrFramebuffer::~HdrFramebuffer()
{
    glDeleteFramebuffers(1, &m_framebuffer);
    glDeleteTextures(1, &m_colorTexture);
    glDeleteTextures(1, &m_depthTexture);
    glDeleteTextures(1, &m_normalTexture);
    glDeleteVertexArrays(1, &m_emptyVao);
}

void HdrFramebuffer::resize(const glm::ivec2& size)
{
    if (size == m_size)
        return;
    m_size = size;

    // allocate texture memory at the new size
    glBindTexture(GL_TEXTURE_2D, m_colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, size.x, size.y, 0, GL_RGBA, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, m_normalTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, size.x, size.y, 0, GL_RGBA, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, m_depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, size.x, size.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
}

void HdrFramebuffer::bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
}

void HdrFramebuffer::bindColorTexture(GLenum textureSlot) const
{
    glActiveTexture(textureSlot);
    glBindTexture(GL_TEXTURE_2D, m_colorTexture);
}

void HdrFramebuffer::bindDepthTexture(GLenum textureSlot) const
{
    glActiveTexture(textureSlot);
    glBindTexture(GL_TEXTURE_2D, m_depthTexture);
}

void HdrFramebuffer::bindNormalTexture(GLenum textureSlot) const
{
    glActiveTexture(textureSlot);
    glBindTexture(GL_TEXTURE_2D, m_normalTexture);
}

void HdrFramebuffer::drawFullscreenTriangle() const
{
    glBindVertexArray(m_emptyVao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}
