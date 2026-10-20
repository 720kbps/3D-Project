#pragma once
#include <framework/opengl_includes.h>
#include <array>
#include <filesystem>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

struct ImageLoadingException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

// How a texture's values are meant to be read.
//  Srgb:   colour images (albedo, emissive). Image files store colour gamma-encoded; the GPU decodes it to
//          linear when sampling, so shaders can do lighting maths on it.
//  Linear: data (normal, roughness, metallic, AO maps). Already linear; decoding it would bend the values.
enum class ColorSpace {
    Srgb,
    Linear
};

// An OpenGL texture that owns its GPU object: either a 2D image or a cube map.
class Texture {
public:
    // 2D texture from an image file. Flipped on load so UV (0, 0) is the image's bottom-left corner.
    Texture(const std::filesystem::path& filePath, ColorSpace colorSpace);
    // Cube map from six square images in OpenGL's face order: +X, -X, +Y, -Y, +Z, -Z. Not flipped
    // (cube map faces use a top-left origin). Sample it with a direction vector, e.g. for skyboxes and reflections.
    Texture(const std::array<std::filesystem::path, 6>& faceFiles, ColorSpace colorSpace);

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    // Moving hands the GPU object over; the moved-from Texture owns nothing, so the object is deleted exactly once.
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;
    ~Texture();

    // textureSlot is GL_TEXTURE0 + unit.
    void bind(GLenum textureSlot) const;
    [[nodiscard]] GLuint id() const { return m_texture; }

private:
    GLenum m_target { GL_TEXTURE_2D };
    GLuint m_texture { 0 }; // 0 = none; OpenGL never uses 0 for a generated texture and glDeleteTextures ignores it.
};

// Loads each (file, colour space) pair once, so materials that share an image share one GPU copy.
// Returned references stay valid for as long as the cache exists.
class TextureCache {
public:
    const Texture& get(const std::filesystem::path& filePath, ColorSpace colorSpace);

private:
    std::map<std::pair<std::string, ColorSpace>, std::unique_ptr<Texture>> m_textures;
};
