#include "texture.h"
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <stb/stb_image.h>
DISABLE_WARNINGS_POP()

namespace {
void loadAndUpload(GLenum target, const std::filesystem::path& filePath, ColorSpace colorSpace, bool flipVertically)
{
    // load image
    stbi_set_flip_vertically_on_load(flipVertically);
    int width, height, channels;
    stbi_uc* pixels = stbi_load(filePath.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels)
        throw ImageLoadingException("Could not load " + filePath.string());

    // upload to GPU
    const GLint internalFormat = colorSpace == ColorSpace::Srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;
    glTexImage2D(target, 0, internalFormat, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    stbi_image_free(pixels);
}
}

Texture::Texture(const std::filesystem::path& filePath, ColorSpace colorSpace)
    : m_target(GL_TEXTURE_2D)
{
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    loadAndUpload(GL_TEXTURE_2D, filePath, colorSpace, true);

    // filtering and mipmaps
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
}

Texture::Texture(const std::array<std::filesystem::path, 6>& faceFiles, ColorSpace colorSpace)
    : m_target(GL_TEXTURE_CUBE_MAP)
{
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_texture);
    for (size_t i = 0; i < faceFiles.size(); ++i)
        loadAndUpload(GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<GLenum>(i), faceFiles[i], colorSpace, false);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
}

Texture::~Texture()
{
    glDeleteTextures(1, &m_texture);
}

void Texture::bind(GLenum textureSlot) const
{
    glActiveTexture(textureSlot);
    glBindTexture(m_target, m_texture);
}
