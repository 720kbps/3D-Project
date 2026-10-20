#include "texture.h"
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <stb/stb_image.h> // The implementation is compiled into the framework (image.cpp).
DISABLE_WARNINGS_POP()
#include <algorithm>
#include <cstring>
#include <string_view>
#include <vector>

namespace {
// GL_EXT_texture_filter_anisotropic is not in this framework's glad loader, so its two constants are
// defined here (values from the extension specification).
constexpr GLenum TEXTURE_MAX_ANISOTROPY_EXT = 0x84FE;
constexpr GLenum MAX_TEXTURE_MAX_ANISOTROPY_EXT = 0x84FF;

struct Pixels {
    int width { 0 };
    int height { 0 };
    int channels { 0 };
    std::vector<unsigned char> data;
};

// stb_image returns rows top to bottom, and OpenGL puts the first row it receives at t = 0.
// flipVertically uploads the bottom row first, so that UV (0, 0) is the image's bottom-left corner.
Pixels loadPixels(const std::filesystem::path& filePath, bool flipVertically)
{
    Pixels result;
    unsigned char* raw = stbi_load(filePath.string().c_str(), &result.width, &result.height, &result.channels, 0);
    if (!raw)
        throw ImageLoadingException("Could not load " + filePath.string() + ": " + stbi_failure_reason());

    const size_t rowBytes = static_cast<size_t>(result.width) * static_cast<size_t>(result.channels);
    const size_t rows = static_cast<size_t>(result.height);
    result.data.resize(rowBytes * rows);
    for (size_t row = 0; row < rows; ++row) {
        const size_t sourceRow = flipVertically ? rows - 1 - row : row;
        std::memcpy(result.data.data() + rowBytes * row, raw + rowBytes * sourceRow, rowBytes);
    }
    stbi_image_free(raw);
    return result;
}

// OpenGL only has sRGB formats with 3 or 4 channels, so grey (+ alpha) colour images become RGB(A).
void expandGreyToColor(Pixels& pixels)
{
    if (pixels.channels > 2)
        return;
    const size_t inChannels = static_cast<size_t>(pixels.channels);
    const size_t outChannels = inChannels == 2 ? 4 : 3;
    const size_t count = static_cast<size_t>(pixels.width) * static_cast<size_t>(pixels.height);
    std::vector<unsigned char> out(count * outChannels);
    for (size_t i = 0; i < count; ++i) {
        const unsigned char grey = pixels.data[i * inChannels];
        out[i * outChannels + 0] = out[i * outChannels + 1] = out[i * outChannels + 2] = grey;
        if (inChannels == 2)
            out[i * outChannels + 3] = pixels.data[i * inChannels + 1];
    }
    pixels.data = std::move(out);
    pixels.channels = static_cast<int>(outChannels);
}

void upload(GLenum target, const Pixels& pixels, ColorSpace colorSpace)
{
    const bool srgb = colorSpace == ColorSpace::Srgb;
    GLint internalFormat = GL_RGBA8;
    GLenum format = GL_RGBA;
    switch (pixels.channels) {
    case 1:
        internalFormat = GL_R8;
        format = GL_RED;
        break;
    case 2:
        internalFormat = GL_RG8;
        format = GL_RG;
        break;
    case 3:
        internalFormat = srgb ? GL_SRGB8 : GL_RGB8;
        format = GL_RGB;
        break;
    default:
        internalFormat = srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;
        format = GL_RGBA;
        break;
    }
    // OpenGL assumes every row starts on a 4-byte boundary; 1- and 3-channel rows often don't.
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(target, 0, internalFormat, pixels.width, pixels.height, 0, format, GL_UNSIGNED_BYTE, pixels.data.data());
}

// Largest anisotropy to use, or 1 if the extension is missing. Queried once.
float maxAnisotropy()
{
    static const float value = [] {
        GLint count = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &count);
        for (GLint i = 0; i < count; ++i) {
            const auto* name = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, static_cast<GLuint>(i)));
            if (name && std::string_view(name) == "GL_EXT_texture_filter_anisotropic") {
                GLfloat max = 1.0f;
                glGetFloatv(MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max);
                return std::min(max, 16.0f);
            }
        }
        return 1.0f;
    }();
    return value;
}
}

Texture::Texture(const std::filesystem::path& filePath, ColorSpace colorSpace)
    : m_target(GL_TEXTURE_2D)
{
    Pixels pixels = loadPixels(filePath, true);
    if (colorSpace == ColorSpace::Srgb)
        expandGreyToColor(pixels);

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    upload(GL_TEXTURE_2D, pixels, colorSpace);

    if (pixels.channels <= 2) {
        // Linear grey data: read it as (v, v, v, 1) or (v, v, v, alpha), like an image viewer would,
        // instead of OpenGL's default (v, 0, 0, 1), which shows up red.
        const GLint swizzle[4] = { GL_RED, GL_RED, GL_RED, pixels.channels == 2 ? GL_GREEN : GL_ONE };
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
    }

    // Repeat outside [0, 1] so UVs can tile, and blend between mipmap levels to avoid shimmering at a distance.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // Surfaces seen at a grazing angle (like the ground) cover a long, thin area of the texture per pixel.
    // Plain mipmapping then picks a level that is blurry in both directions; anisotropic filtering takes
    // several samples along the long axis instead and stays sharp.
    if (maxAnisotropy() > 1.0f)
        glTexParameterf(GL_TEXTURE_2D, TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy());
    glGenerateMipmap(GL_TEXTURE_2D);
}

Texture::Texture(const std::array<std::filesystem::path, 6>& faceFiles, ColorSpace colorSpace)
    : m_target(GL_TEXTURE_CUBE_MAP)
{
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_texture);
    try {
        int faceSize = 0;
        for (size_t i = 0; i < faceFiles.size(); ++i) {
            // No flip: the cube map face table in the OpenGL spec puts t = 0 (the first row received) at the
            // top of each face, which is exactly the order image files store and stb_image returns.
            Pixels pixels = loadPixels(faceFiles[i], false);
            if (pixels.width != pixels.height || (i > 0 && pixels.width != faceSize))
                throw ImageLoadingException("Cube map faces must be square and all the same size: " + faceFiles[i].string());
            faceSize = pixels.width;
            if (colorSpace == ColorSpace::Srgb)
                expandGreyToColor(pixels);
            upload(GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<GLenum>(i), pixels, colorSpace);
        }
    } catch (...) {
        glDeleteTextures(1, &m_texture); // The destructor does not run when a constructor throws.
        throw;
    }

    // Clamp each face at its edges; with GL_TEXTURE_CUBE_MAP_SEAMLESS enabled (see Application) filtering
    // then blends across neighbouring faces instead of showing a seam.
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    // Mipmaps let rough reflections sample a blurrier level later (textureLod with a roughness-based level).
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
}

Texture::Texture(Texture&& other) noexcept
    : m_target(other.m_target)
    , m_texture(std::exchange(other.m_texture, 0))
{
}

Texture& Texture::operator=(Texture&& other) noexcept
{
    if (this != &other) {
        glDeleteTextures(1, &m_texture);
        m_target = other.m_target;
        m_texture = std::exchange(other.m_texture, 0);
    }
    return *this;
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

const Texture& TextureCache::get(const std::filesystem::path& filePath, ColorSpace colorSpace)
{
    // lexically_normal so "a/../b.png" and "b.png" count as the same file.
    const auto key = std::make_pair(filePath.lexically_normal().string(), colorSpace);
    auto it = m_textures.find(key);
    if (it == m_textures.end())
        it = m_textures.emplace(key, std::make_unique<Texture>(filePath, colorSpace)).first;
    return *it->second;
}
