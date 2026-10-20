// SCAFFOLDING, not a graded feature. Source: Mihai (Phase 1).
// Loads 2D textures and cube maps; ColorSpace picks sRGB (colour) or linear (data) storage.

#pragma once
#include <framework/opengl_includes.h>
#include <array>
#include <filesystem>
#include <stdexcept>

struct ImageLoadingException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

// How a texture's values are meant to be read.
//  Srgb:   colour images (albedo, emissive). The GPU decodes them to linear when sampling.
//  Linear: data (normal, roughness, metallic, AO maps). Used as-is.
enum class ColorSpace {
    Srgb,
    Linear
};

// An OpenGL texture that owns its GPU object: either a 2D image or a cube map.
class Texture {
public:
    // 2D texture from an image file, flipped on load so UV (0, 0) is the bottom-left corner.
    Texture(const std::filesystem::path& filePath, ColorSpace colorSpace);
    // Cube map from six square images in OpenGL's face order: +X, -X, +Y, -Y, +Z, -Z (not flipped).
    Texture(const std::array<std::filesystem::path, 6>& faceFiles, ColorSpace colorSpace);

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    ~Texture();

    // textureSlot is GL_TEXTURE0 + unit.
    void bind(GLenum textureSlot) const;

private:
    GLenum m_target { GL_TEXTURE_2D };
    GLuint m_texture { 0 };
};
