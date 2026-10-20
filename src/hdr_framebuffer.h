#pragma once
#include <framework/opengl_includes.h>
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
DISABLE_WARNINGS_POP()

// Offscreen render target for the main pass. The scene is drawn here instead of straight to the
// window, so later passes (tonemapping now; bloom, outlines and SSAO later) can read what was rendered.
//  - color  (RGBA16F): HDR light values. Anything above 1 is kept instead of clipped.
//  - normal (RGBA16F): view-space normal in xyz, alpha 1 where geometry was drawn and 0 elsewhere.
//                      RGBA rather than RGB because RGB16F is not guaranteed to be renderable in GL 4.1.
//  - depth  (DEPTH24): a texture rather than a renderbuffer, so post passes can sample it.
class HdrFramebuffer {
public:
    HdrFramebuffer() = default;
    HdrFramebuffer(const HdrFramebuffer&) = delete;
    HdrFramebuffer& operator=(const HdrFramebuffer&) = delete;
    ~HdrFramebuffer();

    // (Re)creates the attachments when the size changes. Cheap to call every frame.
    void resize(const glm::ivec2& size);
    // Binds the target for drawing, sets the viewport, and clears colour (to clearColor), normals (to 0) and depth.
    void bindAndClear(const glm::vec3& clearColor) const;

    [[nodiscard]] GLuint colorTexture() const { return m_color; }
    [[nodiscard]] GLuint normalTexture() const { return m_normal; }
    [[nodiscard]] GLuint depthTexture() const { return m_depth; }
    [[nodiscard]] glm::ivec2 size() const { return m_size; }

private:
    void release();

    // 0 means "no object": OpenGL never hands out name 0, and glDelete* ignores it.
    glm::ivec2 m_size { 0 };
    GLuint m_fbo { 0 };
    GLuint m_color { 0 };
    GLuint m_normal { 0 };
    GLuint m_depth { 0 };
};

// Draws one triangle that covers the whole screen, for full-screen passes. The vertices are generated
// in the vertex shader from gl_VertexID, but core profile still requires a vertex array object to be bound.
class FullscreenTriangle {
public:
    FullscreenTriangle();
    FullscreenTriangle(const FullscreenTriangle&) = delete;
    FullscreenTriangle& operator=(const FullscreenTriangle&) = delete;
    ~FullscreenTriangle();

    void draw() const;

private:
    GLuint m_vao { 0 };
};
