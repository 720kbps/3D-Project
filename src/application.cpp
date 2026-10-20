// Main file: window, scene list, render loop and the settings UI.
// Source: course starter structure (scaffolding).
// Both of you edit this file, but keep it thin: a feature lives in src/features/ and only gets
// called from here (setup in buildScene, per-frame update + draw in update(), toggle in drawUI).

#include "core/camera.h"
#include "core/mesh.h"
#include "core/primitives.h"
#include "core/texture.h"
// Always include window first (because it includes glfw, which includes GL which needs to be included AFTER glew).
// Can't wait for modules to fix this stuff...
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glad/glad.h>
// Include glad before glfw3
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/color_space.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <imgui/imgui.h>
DISABLE_WARNINGS_POP()
#include <framework/shader.h>
#include <framework/window.h>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

// One thing to draw: which meshes, where, and with which colour or texture.
// The meshes and textures are owned by the Application, objects only point at them.
struct SceneObject {
    std::string name;
    const std::vector<GPUMesh>* meshes;
    glm::mat4 modelMatrix { 1.0f };
    glm::vec3 baseColor { 0.8f }; // Linear colour.
    const Texture* albedoMap { nullptr }; // Replaces baseColor when set.
    bool visible { true };
};

// Wraps one generated primitive (primitives.h) in the same container type a loaded .obj file gives.
static std::vector<GPUMesh> toGPU(const Mesh& mesh)
{
    std::vector<GPUMesh> result;
    result.emplace_back(mesh);
    return result;
}

// Colours are picked as they look on screen (sRGB) and converted to the linear values the shader works in.
static glm::vec3 srgb(float r, float g, float b)
{
    return glm::convertSRGBToLinear(glm::vec3(r, g, b));
}

class Application {
public:
    Application()
        : m_window("Final Project", glm::ivec2(1024, 1024), OpenGLVersion::GL41)
        , m_camera(&m_window, glm::vec3(3.0f, 2.0f, 3.0f), -glm::vec3(3.0f, 2.0f, 3.0f))
        , m_checkerTexture(RESOURCE_ROOT "resources/checkerboard.png", ColorSpace::Srgb)
    {
        m_window.registerKeyCallback([this](int key, int /* scancode */, int action, int mods) {
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

        // Lets cube map filtering blend across face edges (no seams). Needed once environment mapping is in.
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

            // Any new shaders can be added below in similar fashion.
            // ==> Don't forget to reconfigure CMake when you do!
            //     Visual Studio: PROJECT => Generate Cache for ComputerGraphics
            //     VS Code: ctrl + shift + p => CMake: Configure => enter
            // ....
        } catch (const ShaderLoadingException& e) {
            std::cerr << e.what() << std::endl;
        }
    }

    void update()
    {
        while (!m_window.shouldClose()) {
            // This is your game loop
            // Put your real-time logic and rendering in here
            m_window.updateInput();

            // Time in seconds since the last frame. Animate with this, never per frame, so speed is the same on every computer.
            const float now = static_cast<float>(glfwGetTime());
            m_deltaTime = now - m_lastFrameTime;
            m_lastFrameTime = now;

            drawUI();

            // Don't rotate the camera while dragging an ImGui slider.
            m_camera.setUserInteraction(!ImGui::GetIO().WantCaptureMouse);
            m_camera.updateInput();

            // On a Retina screen the framebuffer has 2x the pixels of the window, so use its size for the viewport.
            const glm::ivec2 framebufferSize = m_window.getFrameBufferSize();
            const float aspectRatio = framebufferSize.y > 0 ? static_cast<float>(framebufferSize.x) / static_cast<float>(framebufferSize.y) : 1.0f;
            const glm::mat4 viewMatrix = m_camera.viewMatrix();
            const glm::mat4 projectionMatrix = glm::perspective(glm::radians(60.0f), aspectRatio, 0.1f, 50.0f);

            // Clear the screen
            glViewport(0, 0, framebufferSize.x, framebufferSize.y);
            glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);

            m_defaultShader.bind();
            for (const SceneObject& object : m_objects) {
                if (!object.visible)
                    continue;

                const glm::mat4 mvpMatrix = projectionMatrix * viewMatrix * object.modelMatrix;
                // Normals should be transformed differently than positions (ignoring translations + dealing with scaling):
                // https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html
                const glm::mat3 normalModelMatrix = glm::inverseTranspose(glm::mat3(object.modelMatrix));
                glUniformMatrix4fv(m_defaultShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
                glUniformMatrix4fv(m_defaultShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(object.modelMatrix));
                glUniformMatrix3fv(m_defaultShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrix));

                glUniform3fv(m_defaultShader.getUniformLocation("baseColor"), 1, glm::value_ptr(object.baseColor));
                const bool useTexture = object.albedoMap && m_useTextures;
                glUniform1i(m_defaultShader.getUniformLocation("hasAlbedoMap"), useTexture);
                if (useTexture) {
                    object.albedoMap->bind(GL_TEXTURE0);
                    glUniform1i(m_defaultShader.getUniformLocation("albedoMap"), 0);
                }

                for (const GPUMesh& mesh : *object.meshes)
                    mesh.draw();
            }

            // Processes input and swaps the window buffer
            m_window.swapBuffers();
        }
    }

    // In here you can handle key presses
    // key - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__keys.html
    // mods - Any modifier keys pressed, like shift or control
    void onKeyPressed(int /* key */, int /* mods */) { }

    // In here you can handle key releases
    void onKeyReleased(int /* key */, int /* mods */) { }

    // If the mouse is moved this function will be called with the x, y screen-coordinates of the mouse
    void onMouseMove(const glm::dvec2& /* cursorPos */) { }

    // If one of the mouse buttons is pressed this function will be called
    // button - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__buttons.html
    void onMouseClicked(int /* button */, int /* mods */) { }

    // If one of the mouse buttons is released this function will be called
    void onMouseReleased(int /* button */, int /* mods */) { }

private:
    // Test layout: the dragon in the middle of a ground plane, spheres on one side, a textured cube and a
    // cylinder on the other. Units are roughly metres; the dragon is about 1 long.
    void buildScene()
    {
        m_planeMesh = toGPU(makePlane(6.0f));
        m_cubeMesh = toGPU(makeCube());
        m_sphereMesh = toGPU(makeUVSphere());
        m_cylinderMesh = toGPU(makeCylinder());
        m_dragonMesh = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/dragon.obj");

        m_objects.push_back({ "Ground", &m_planeMesh, glm::scale(glm::mat4(1.0f), glm::vec3(6.0f, 1.0f, 6.0f)), srgb(0.45f, 0.47f, 0.5f) });
        // The dragon's lowest point is at y = -0.283, so lift it to stand on the ground.
        m_objects.push_back({ "Dragon", &m_dragonMesh, glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.283f, 0.0f)), srgb(0.85f, 0.7f, 0.35f) });

        const glm::vec3 sphereColors[] = { srgb(0.8f, 0.2f, 0.2f), srgb(0.2f, 0.7f, 0.3f), srgb(0.2f, 0.4f, 0.85f), srgb(0.9f, 0.8f, 0.2f) };
        for (int i = 0; i < 4; ++i) {
            const glm::vec3 position(1.3f, 0.3f, -1.2f + 0.8f * static_cast<float>(i));
            const glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), position), glm::vec3(0.6f));
            m_objects.push_back({ "Sphere " + std::to_string(i + 1), &m_sphereMesh, model, sphereColors[i] });
        }

        glm::mat4 cubeModel = glm::translate(glm::mat4(1.0f), glm::vec3(-1.3f, 0.35f, -0.6f));
        cubeModel = glm::rotate(cubeModel, glm::radians(30.0f), glm::vec3(0, 1, 0));
        cubeModel = glm::scale(cubeModel, glm::vec3(0.7f));
        m_objects.push_back({ "Cube", &m_cubeMesh, cubeModel, glm::vec3(1.0f), &m_checkerTexture });

        const glm::mat4 cylinderModel = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-1.3f, 0.4f, 0.8f)), glm::vec3(0.5f, 0.8f, 0.5f));
        m_objects.push_back({ "Cylinder", &m_cylinderMesh, cylinderModel, srgb(0.6f, 0.35f, 0.75f) });
    }

    // Settings panel. Every feature gets a toggle here (the brief requires it).
    void drawUI()
    {
        ImGui::Begin("Settings");
        ImGui::Text("%.1f FPS", static_cast<double>(ImGui::GetIO().Framerate));
        ImGui::TextUnformatted("Camera: WASD move, R/F up/down, hold left mouse to look.");
        ImGui::Checkbox("Use textures", &m_useTextures);
        if (ImGui::CollapsingHeader("Objects")) {
            for (SceneObject& object : m_objects)
                ImGui::Checkbox(object.name.c_str(), &object.visible);
        }
        ImGui::End();
    }

    Window m_window;
    Camera m_camera;

    // Shader for default rendering and for depth rendering
    Shader m_defaultShader;
    Shader m_shadowShader;

    // Meshes and textures, shared by the objects that point at them.
    std::vector<GPUMesh> m_planeMesh, m_cubeMesh, m_sphereMesh, m_cylinderMesh, m_dragonMesh;
    Texture m_checkerTexture;
    std::vector<SceneObject> m_objects;

    bool m_useTextures { true };
    float m_lastFrameTime { 0.0f };
    float m_deltaTime { 0.0f };
};

int main()
{
    Application app;
    app.update();

    return 0;
}
