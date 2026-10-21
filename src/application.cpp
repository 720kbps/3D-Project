#include "core/camera.h"
#include "core/mesh.h"
#include "core/texture.h"
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
// glad before glfw
#include <glad/glad.h>
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

struct SceneObject {
    std::string name;
    const std::vector<GPUMesh>* meshes;
    glm::mat4 modelMatrix { 1.0f };
    glm::vec3 baseColor { 0.8f };
    const Texture* albedoMap { nullptr };
    bool visible { true };
};

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
        // input callbacks
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

        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

        buildScene();

        // compile shaders
        try {
            ShaderBuilder defaultBuilder;
            defaultBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl");
            defaultBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shader_frag.glsl");
            m_defaultShader = defaultBuilder.build();

            ShaderBuilder shadowBuilder;
            shadowBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shadow_vert.glsl");
            shadowBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shadow_frag.glsl");
            m_shadowShader = shadowBuilder.build();
        } catch (const ShaderLoadingException& e) {
            std::cerr << e.what() << std::endl;
        }
    }

    void update()
    {
        while (!m_window.shouldClose()) {
            m_window.updateInput();

            // delta time (capped)
            const float now = static_cast<float>(glfwGetTime());
            m_deltaTime = glm::min(now - m_lastFrameTime, 0.1f);
            m_lastFrameTime = now;

            // animation time
            if (!m_paused)
                m_time += m_deltaTime;

            drawUI();

            // camera (ignore mouse while over the UI)
            m_camera.setUserInteraction(!ImGui::GetIO().WantCaptureMouse);
            m_camera.updateInput(m_deltaTime);

            // view and projection
            const glm::ivec2 framebufferSize = m_window.getFrameBufferSize();
            const float aspectRatio = framebufferSize.y > 0 ? static_cast<float>(framebufferSize.x) / static_cast<float>(framebufferSize.y) : 1.0f;
            const glm::mat4 viewMatrix = m_camera.viewMatrix();
            const glm::mat4 projectionMatrix = glm::perspective(glm::radians(60.0f), aspectRatio, 0.1f, 50.0f);

            // clear screen
            glViewport(0, 0, framebufferSize.x, framebufferSize.y);
            glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);

            // draw objects
            m_defaultShader.bind();
            for (const SceneObject& object : m_objects) {
                if (!object.visible)
                    continue;

                // matrices
                const glm::mat4 mvpMatrix = projectionMatrix * viewMatrix * object.modelMatrix;
                const glm::mat3 normalModelMatrix = glm::inverseTranspose(glm::mat3(object.modelMatrix));
                glUniformMatrix4fv(m_defaultShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
                glUniformMatrix4fv(m_defaultShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(object.modelMatrix));
                glUniformMatrix3fv(m_defaultShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrix));

                // material
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

            m_window.swapBuffers();
        }
    }

    void onKeyPressed(int /* key */, int /* mods */) { }
    void onKeyReleased(int /* key */, int /* mods */) { }
    void onMouseMove(const glm::dvec2& /* cursorPos */) { }
    void onMouseClicked(int /* button */, int /* mods */) { }
    void onMouseReleased(int /* button */, int /* mods */) { }

private:
    void buildScene()
    {
        // load meshes
        m_planeMesh = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/plane.obj");
        m_cubeMesh = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/cube.obj");
        m_sphereMesh = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/sphere.obj");
        m_cylinderMesh = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/cylinder.obj");
        m_dragonMesh = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/dragon.obj");

        // ground and dragon
        m_objects.push_back({ "Ground", &m_planeMesh, glm::scale(glm::mat4(1.0f), glm::vec3(6.0f, 1.0f, 6.0f)), srgb(0.45f, 0.47f, 0.5f) });
        m_objects.push_back({ "Dragon", &m_dragonMesh, glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.283f, 0.0f)), srgb(0.85f, 0.7f, 0.35f) });

        // spheres
        const glm::vec3 sphereColors[] = { srgb(0.8f, 0.2f, 0.2f), srgb(0.2f, 0.7f, 0.3f), srgb(0.2f, 0.4f, 0.85f), srgb(0.9f, 0.8f, 0.2f) };
        for (int i = 0; i < 4; ++i) {
            const glm::vec3 position(1.3f, 0.3f, -1.2f + 0.8f * static_cast<float>(i));
            const glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), position), glm::vec3(0.6f));
            m_objects.push_back({ "Sphere " + std::to_string(i + 1), &m_sphereMesh, model, sphereColors[i] });
        }

        // cube
        glm::mat4 cubeModel = glm::translate(glm::mat4(1.0f), glm::vec3(-1.3f, 0.35f, -0.6f));
        cubeModel = glm::rotate(cubeModel, glm::radians(30.0f), glm::vec3(0, 1, 0));
        cubeModel = glm::scale(cubeModel, glm::vec3(0.7f));
        m_objects.push_back({ "Cube", &m_cubeMesh, cubeModel, glm::vec3(1.0f), &m_checkerTexture });

        // cylinder
        const glm::mat4 cylinderModel = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-1.3f, 0.4f, 0.8f)), glm::vec3(0.5f, 0.8f, 0.5f));
        m_objects.push_back({ "Cylinder", &m_cylinderMesh, cylinderModel, srgb(0.6f, 0.35f, 0.75f) });
    }

    void drawUI()
    {
        ImGui::Begin("Settings");
        ImGui::Text("%.1f FPS (%.2f ms)", static_cast<double>(ImGui::GetIO().Framerate), static_cast<double>(1000.0f / ImGui::GetIO().Framerate));
        ImGui::TextUnformatted("Camera: WASD move, R/F up/down, hold left mouse to look.");
        ImGui::Checkbox("Use textures", &m_useTextures);
        if (ImGui::CollapsingHeader("Time")) {
            ImGui::Text("Time: %.2f s", static_cast<double>(m_time));
            ImGui::Checkbox("Pause", &m_paused);
            if (ImGui::Checkbox("VSync", &m_vsync))
                glfwSwapInterval(m_vsync ? 1 : 0);
        }
        if (ImGui::CollapsingHeader("Objects")) {
            for (SceneObject& object : m_objects)
                ImGui::Checkbox(object.name.c_str(), &object.visible);
        }
        ImGui::End();
    }

    Window m_window;
    Camera m_camera;

    Shader m_defaultShader;
    Shader m_shadowShader;

    std::vector<GPUMesh> m_planeMesh, m_cubeMesh, m_sphereMesh, m_cylinderMesh, m_dragonMesh;
    Texture m_checkerTexture;
    std::vector<SceneObject> m_objects;

    bool m_useTextures { true };
    float m_lastFrameTime { 0.0f };
    float m_deltaTime { 0.0f };
    float m_time { 0.0f };
    bool m_paused { false };
    bool m_vsync { true };
};

int main()
{
    Application app;
    app.update();

    return 0;
}
