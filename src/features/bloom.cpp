#include "bloom.h"
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/common.hpp>
DISABLE_WARNINGS_POP()

Bloom::Bloom()
{
    // two textures, so the blur can read one while writing the other
    glGenTextures(2, m_textures);
    glGenFramebuffers(2, m_framebuffers);
    for (int i = 0; i < 2; ++i) {
        glBindTexture(GL_TEXTURE_2D, m_textures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffers[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_textures[i], 0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Bloom::~Bloom()
{
    glDeleteFramebuffers(2, m_framebuffers);
    glDeleteTextures(2, m_textures);
}

void Bloom::resize(const glm::ivec2& screenSize)
{
    // half resolution: cheaper, and the blur reaches further
    const glm::ivec2 size = glm::max(screenSize / 2, glm::ivec2(1));
    if (size == m_size)
        return;
    m_size = size;

    for (int i = 0; i < 2; ++i) {
        glBindTexture(GL_TEXTURE_2D, m_textures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, size.x, size.y, 0, GL_RGBA, GL_FLOAT, nullptr);
    }
}

void Bloom::bindTarget(int index) const
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffers[index]);
    glViewport(0, 0, m_size.x, m_size.y);
}

void Bloom::bindTexture(int index, GLenum textureSlot) const
{
    glActiveTexture(textureSlot);
    glBindTexture(GL_TEXTURE_2D, m_textures[index]);
}
