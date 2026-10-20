//#include "Image.h"
#include "frame_clock.h"
#include "gl_check.h"
#include "hdr_framebuffer.h"
#include "mesh.h"
#include "primitives.h"
#include "scene.h"
#include "screenshot.h"
#include "settings.h"
#include "texture.h"
#include "ui.h"
// Always include window first (because it includes glfw, which includes GL which needs to be included AFTER glew).
// Can't wait for modules to fix this stuff...
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glad/glad.h>
// Include glad before glfw3
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/color_space.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <imgui/imgui.h>
DISABLE_WARNINGS_POP()
#include <framework/shader.h>
#include <framework/trackball.h>
#include <framework/window.h>
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <functional>
#include <iostream>
#include <optional>
#include <string_view>
#include <thread>
#include <vector>

class Application {
public:
    Application()
        : m_window("Final Project", glm::ivec2(1024, 1024), OpenGLVersion::GL41)
        // Stopgap camera until the real camera system (task 2.1): orbits the look-at point, starting
        // 4.5 units away, 25 degrees above the ground and 30 degrees round to the side.
        , m_trackball(&m_window, glm::radians(60.0f), glm::vec3(0.0f, 0.3f, 0.0f), 4.5f, glm::radians(25.0f), glm::radians(30.0f))
        , m_debugCubemap(debugCubemapFaces(), ColorSpace::Srgb)
    {
        m_window.registerKeyCallback([this](int key, int scancode, int action, int mods) {
            if (action == GLFW_PRESS)
                onKeyPressed(key, mods);
            else if (action == GLFW_RELEASE)
                onKeyReleased(key, mods);
        });
        m_window.registerMouseMoveCallback(std::bind(&Application::onMouseMove, this, std::placeholders::_1));
        m_window.registerMouseButtonCallback([this](int button, int action, int mods) {
            if (action == GLFW_PRESS)
                onMouseClicked(button, mods);
            else if (action == GLFW_RELEASE)
                onMouseReleased(button, mods);
        });

        // Let cube map filtering blend across face edges instead of clamping each face separately (no visible seams).
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

        buildScene();

        try {
            ShaderBuilder defaultBuilder;
            defaultBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl");
            defaultBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shader_frag.glsl");
            m_defaultShader = defaultBuilder.build();

            ShaderBuilder shadowBuilder;
            shadowBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shadow_vert.glsl");
            shadowBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shadow_frag.glsl");
            m_shadowShader = shadowBuilder.build();

            ShaderBuilder tonemapBuilder;
            tonemapBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/fullscreen_vert.glsl");
            tonemapBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/tonemap_frag.glsl");
            m_tonemapShader = tonemapBuilder.build();

            // Any new shaders can be added below in similar fashion.
            // ==> Don't forget to reconfigure CMake when you do!
            //     Visual Studio: PROJECT => Generate Cache for ComputerGraphics
            //     VS Code: ctrl + shift + p => CMake: Configure => enter
            // ....
        } catch (const ShaderLoadingException& e) {
            std::cerr << e.what() << std::endl;
        }
        checkGLErrors("startup");
    }

    void update()
    {
        while (!m_window.shouldClose()) {
            // This is your game loop
            // Put your real-time logic and rendering in here
            m_window.updateInput();

            // Advance the clock first so everything below sees this frame's delta time.
            m_clock.tick(glfwGetTime(), m_settings.paused, m_settings.timeScale);
            // Debug aid: an artificially slow frame, to check that animation speed does not depend on FPS.
            if (m_settings.simulatedLagMs > 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(m_settings.simulatedLagMs));

            drawSettingsUI(m_settings, m_clock, m_scene);
            // The UI only edits the flag; apply it here, and only when it changed.
            if (m_settings.vsync != m_appliedVsync) {
                glfwSwapInterval(m_settings.vsync ? 1 : 0);
                m_appliedVsync = m_settings.vsync;
            }

            // Temporary dt test: 45 degrees per second of simulation time, so one full turn every 8 seconds at any FPS.
            if (Entity* dragon = m_scene.find("Dragon")) {
                const float spinAngle = m_settings.spinTest ? static_cast<float>(m_clock.simTime()) * glm::radians(45.0f) : 0.0f;
                dragon->transform.rotation = glm::angleAxis(spinAngle, glm::vec3(0, 1, 0));
            }

            // The framebuffer is measured in pixels, which on a Retina screen is 2x the window size in points.
            // Reading it every frame keeps the render target, viewport and aspect ratio correct after a resize.
            const glm::ivec2 framebufferSize = m_window.getFrameBufferSize();
            const float aspectRatio = framebufferSize.y > 0 ? static_cast<float>(framebufferSize.x) / static_cast<float>(framebufferSize.y) : 1.0f; // y is 0 while minimized
            const glm::mat4 viewMatrix = m_trackball.viewMatrix();
            const glm::mat4 projectionMatrix = m_trackball.projectionMatrix(aspectRatio);

            // Main pass: draw the scene into the offscreen HDR target instead of the window (bindAndClear also sets the viewport).
            m_hdrTarget.resize(framebufferSize);
            m_hdrTarget.bindAndClear(CLEAR_COLOR);
            glEnable(GL_DEPTH_TEST);

            // Per-frame uniforms first; drawScene then sets the per-entity ones.
            m_defaultShader.bind();
            glUniform1i(m_defaultShader.getUniformLocation("debugView"), static_cast<int>(m_settings.debugView));
            glUniform3fv(m_defaultShader.getUniformLocation("cameraPosition"), 1, glm::value_ptr(m_trackball.position()));
            glUniform1f(m_defaultShader.getUniformLocation("depthViewRange"), m_settings.depthViewRange);
            glUniformMatrix4fv(m_defaultShader.getUniformLocation("viewMatrix"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
            // Always bound, even when the cubemap view is off: the samplerCube would otherwise default to unit 0,
            // where the albedo map's sampler2D lives, and two sampler types on one unit make the draw fail.
            m_debugCubemap.bind(GL_TEXTURE0 + TextureUnit::DebugCubemap);
            glUniform1i(m_defaultShader.getUniformLocation("debugCubemap"), TextureUnit::DebugCubemap);
            drawScene(m_scene, m_defaultShader, viewMatrix, projectionMatrix);
            checkGLErrors("main pass");

            // Final pass: back to the window's framebuffer, one full-screen triangle that reads the HDR target.
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, framebufferSize.x, framebufferSize.y);
            glDisable(GL_DEPTH_TEST);
            m_tonemapShader.bind();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_hdrTarget.colorTexture());
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, m_hdrTarget.normalTexture());
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, m_hdrTarget.depthTexture());
            glUniform1i(m_tonemapShader.getUniformLocation("hdrColor"), 0);
            glUniform1i(m_tonemapShader.getUniformLocation("viewNormals"), 1);
            glUniform1i(m_tonemapShader.getUniformLocation("depthBuffer"), 2);
            glUniform1f(m_tonemapShader.getUniformLocation("exposure"), std::exp2(m_settings.exposureEV));
            glUniform1i(m_tonemapShader.getUniformLocation("tonemapper"), static_cast<int>(m_settings.tonemapper));
            glUniform1i(m_tonemapShader.getUniformLocation("srgbEncode"), m_settings.srgbEncode);
            // Debug views write raw values meant to be seen as-is, so they skip exposure, tonemapping and encoding.
            glUniform1i(m_tonemapShader.getUniformLocation("passthrough"), m_settings.debugView != DebugView::Default);
            glUniform1i(m_tonemapShader.getUniformLocation("bufferView"), static_cast<int>(m_settings.bufferView));
            glUniform2f(m_tonemapShader.getUniformLocation("depthLinearize"), projectionMatrix[2][2], projectionMatrix[3][2]);
            glUniform1f(m_tonemapShader.getUniformLocation("depthViewRange"), m_settings.depthViewRange);
            m_fullscreenTriangle.draw();
            glActiveTexture(GL_TEXTURE0);
            checkGLErrors("tonemap pass");

            // Read the frame back before swapBuffers: the back buffer still holds it, and the UI
            // (drawn inside swapBuffers) is not in it yet, which is what report screenshots want.
            if (m_screenshotRequested || (m_capturePath && ++m_capturedFrames == CAPTURE_AFTER_FRAMES)) {
                const std::filesystem::path path = m_capturePath ? *m_capturePath : nextFreeScreenshotPath(RESOURCE_ROOT "docs/screens");
                if (saveScreenshot(path, framebufferSize.x, framebufferSize.y))
                    std::cout << "Saved " << path.string() << std::endl;
                else
                    std::cerr << "Could not save screenshot to " << path.string() << std::endl;
                m_screenshotRequested = false;
                if (m_capturePath)
                    m_window.close();
            }

            // Processes input and swaps the window buffer
            m_window.swapBuffers();
        }
    }

    Settings& settings() { return m_settings; }

    // Non-interactive mode: render a few frames, save one to filePath and quit.
    void captureAndQuit(const std::filesystem::path& filePath) { m_capturePath = filePath; }

    // In here you can handle key presses
    // key - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__keys.html
    // mods - Any modifier keys pressed, like shift or control
    void onKeyPressed(int key, int /* mods */)
    {
        if (key == GLFW_KEY_F12)
            m_screenshotRequested = true;
    }

    // In here you can handle key releases
    // key - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__keys.html
    // mods - Any modifier keys pressed, like shift or control
    void onKeyReleased(int /* key */, int /* mods */)
    {
    }

    // If the mouse is moved this function will be called with the x, y screen-coordinates of the mouse
    void onMouseMove(const glm::dvec2& /* cursorPos */)
    {
    }

    // If one of the mouse buttons is pressed this function will be called
    // button - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__buttons.html
    // mods - Any modifier buttons pressed
    void onMouseClicked(int /* button */, int /* mods */)
    {
    }

    // If one of the mouse buttons is released this function will be called
    // button - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__buttons.html
    // mods - Any modifier buttons pressed
    void onMouseReleased(int /* button */, int /* mods */)
    {
    }

private:
    // Test layout: the dragon in the middle of a ground plane, a row of spheres on one side (for
    // material comparisons later) and a textured cube and a cylinder on the other. Units are metres-ish:
    // the dragon is about 1 long.
    void buildScene()
    {
        // Colours are picked as they should look on screen (sRGB) and converted to the linear values the shaders work in.
        const auto srgb = [](float r, float g, float b) { return glm::convertSRGBToLinear(glm::vec3(r, g, b)); };

        SurfaceMaterial ground;
        ground.baseColor = srgb(0.45f, 0.47f, 0.5f);
        m_scene.addEntity("Ground", m_scene.addMesh(makePlane(6.0f)), { .scale = glm::vec3(6.0f, 1.0f, 6.0f) }, ground);

        // The dragon model's lowest point is at y = -0.283, so lift it to stand on the ground.
        SurfaceMaterial dragon;
        dragon.baseColor = srgb(0.85f, 0.7f, 0.35f);
        m_scene.addEntity("Dragon", m_scene.addMeshes(GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/dragon.obj")), { .position = glm::vec3(0.0f, 0.283f, 0.0f) }, dragon);

        const auto* sphereMesh = m_scene.addMesh(makeUVSphere());
        const glm::vec3 sphereColors[] = { { 0.8f, 0.2f, 0.2f }, { 0.2f, 0.7f, 0.3f }, { 0.2f, 0.4f, 0.85f }, { 0.9f, 0.8f, 0.2f } };
        for (int i = 0; i < 4; ++i) {
            SurfaceMaterial material;
            material.baseColor = glm::convertSRGBToLinear(sphereColors[i]);
            const float z = -1.2f + 0.8f * static_cast<float>(i);
            m_scene.addEntity("Sphere " + std::to_string(i + 1), sphereMesh, { .position = glm::vec3(1.3f, 0.3f, z), .scale = glm::vec3(0.6f) }, material);
        }

        SurfaceMaterial checker;
        checker.baseColor = glm::vec3(1.0f);
        checker.albedoMap = &m_textures.get(RESOURCE_ROOT "resources/checkerboard.png", ColorSpace::Srgb);
        m_scene.addEntity("Cube", m_scene.addMesh(makeCube()),
            { .position = glm::vec3(-1.3f, 0.35f, -0.6f), .rotation = glm::angleAxis(glm::radians(30.0f), glm::vec3(0, 1, 0)), .scale = glm::vec3(0.7f) }, checker);

        SurfaceMaterial cylinder;
        cylinder.baseColor = srgb(0.6f, 0.35f, 0.75f);
        m_scene.addEntity("Cylinder", m_scene.addMesh(makeCylinder()), { .position = glm::vec3(-1.3f, 0.4f, 0.8f), .scale = glm::vec3(0.5f, 0.8f, 0.5f) }, cylinder);

        // Texture test sign behind the dragon. Seen from the start camera it must read: red top-left, green
        // top-right, blue bottom-left, yellow bottom-right, with an sRGB 128 grey patch in the middle that stays
        // exactly 128 on screen when the tonemapper is None. The plane faces +Y; tilt it upright (+90 degrees
        // about X, so the texture's up becomes +Y), then turn it 180 degrees about Y to face the camera.
        SurfaceMaterial sign;
        sign.baseColor = glm::vec3(1.0f);
        sign.albedoMap = &m_textures.get(RESOURCE_ROOT "resources/debug/orientation.png", ColorSpace::Srgb);
        const glm::quat upright = glm::angleAxis(glm::pi<float>(), glm::vec3(0, 1, 0)) * glm::angleAxis(glm::half_pi<float>(), glm::vec3(1, 0, 0));
        m_scene.addEntity("Texture test", m_scene.addMesh(makePlane()), { .position = glm::vec3(0.0f, 1.1f, 2.2f), .rotation = upright }, sign);
    }

    static std::array<std::filesystem::path, 6> debugCubemapFaces()
    {
        const std::filesystem::path folder = RESOURCE_ROOT "resources/debug/cubemap";
        return { folder / "px.png", folder / "nx.png", folder / "py.png", folder / "ny.png", folder / "pz.png", folder / "nz.png" };
    }

    Window m_window;
    Trackball m_trackball;

    // Shader for default rendering and for depth rendering
    Shader m_defaultShader;
    Shader m_shadowShader;

    // Offscreen HDR target and the final full-screen pass (exposure, tonemapping, sRGB encoding).
    HdrFramebuffer m_hdrTarget;
    FullscreenTriangle m_fullscreenTriangle;
    Shader m_tonemapShader;
    // The old window clear colour (0.2 grey as seen on screen), converted to linear because the HDR target holds linear values.
    inline static const glm::vec3 CLEAR_COLOR = glm::convertSRGBToLinear(glm::vec3(0.2f));

    // Declared before the scene, which points into it.
    TextureCache m_textures;
    Texture m_debugCubemap;
    Scene m_scene;

    Settings m_settings;
    FrameClock m_clock;
    bool m_appliedVsync { true }; // The framework enables vsync when it creates the window.

    bool m_screenshotRequested { false };
    // Capture mode: wait a few frames so ImGui and the frame timing have settled before saving.
    static constexpr int CAPTURE_AFTER_FRAMES = 30;
    std::optional<std::filesystem::path> m_capturePath;
    int m_capturedFrames { 0 };
};

// Optional command line, for repeatable screenshots (report figures, or checking a change without clicking):
//   Master_TechDemo --view normals --capture out.png
// --view takes default, normals, uvs, depth, cubemap, tangents or tangentsign; --tonemap none, reinhard or aces;
// --buffer final, normals or depth. Without --capture the demo runs normally.
int main(int argc, char** argv)
{
    Application app;
    const std::vector<std::string_view> args(argv + 1, argv + argc);
    for (size_t i = 0; i + 1 < args.size(); i += 2) {
        const std::string_view option = args[i], value = args[i + 1];
        if (option == "--capture") {
            app.captureAndQuit(std::filesystem::path(value));
        } else if (option == "--view") {
            if (value == "normals")
                app.settings().debugView = DebugView::Normals;
            else if (value == "uvs")
                app.settings().debugView = DebugView::TexCoords;
            else if (value == "depth")
                app.settings().debugView = DebugView::Depth;
            else if (value == "cubemap")
                app.settings().debugView = DebugView::Cubemap;
            else if (value == "tangents")
                app.settings().debugView = DebugView::Tangents;
            else if (value == "tangentsign")
                app.settings().debugView = DebugView::TangentSign;
            else
                app.settings().debugView = DebugView::Default;
        // --tonemap none|reinhard|aces, --buffer final|normals|depth
        } else if (option == "--tonemap") {
            app.settings().tonemapper = value == "aces" ? Tonemapper::Aces : value == "reinhard" ? Tonemapper::Reinhard : Tonemapper::None;
        } else if (option == "--buffer") {
            app.settings().bufferView = value == "normals" ? BufferView::Normals : value == "depth" ? BufferView::Depth : BufferView::Final;
        } else {
            std::cerr << "Unknown option " << option << std::endl;
        }
    }
    app.update();

    return 0;
}
