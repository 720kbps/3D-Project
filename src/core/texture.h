#pragma once
#include <framework/opengl_includes.h>
#include <array>
#include <filesystem>
#include <stdexcept>

struct ImageLoadingException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

enum class ColorSpace {
    Srgb,
    Linear
};

class Texture {
public:
    Texture(const std::filesystem::path& filePath, ColorSpace colorSpace);
    // faces: +X, -X, +Y, -Y, +Z, -Z
    Texture(const std::array<std::filesystem::path, 6>& faceFiles, ColorSpace colorSpace);

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    ~Texture();

    void bind(GLenum textureSlot) const;

private:
    GLenum m_target { GL_TEXTURE_2D };
    GLuint m_texture { 0 };
};
