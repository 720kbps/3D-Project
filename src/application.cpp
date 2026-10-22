#include "core/camera.h"
#include "core/hdr_framebuffer.h"
#include "core/mesh.h"
#include "core/texture.h"
#include "features/bezier_path.h"
#include "features/chase_camera.h"
#include "features/orbit_camera.h"
#include "features/top_camera.h"
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

struct PointLight {
    glm::vec3 position;
    glm::vec3 color;
    float intensity { 2.0f };
    bool enabled { false };
};

struct SceneObject {
    std::string name;
    const std::vector<GPUMesh>* meshes;
    glm::mat4 modelMatrix { 1.0f };
    glm::vec3 baseColor { 0.8f };
    const Texture* albedoMap { nullptr };
    const Texture* roughnessMap { nullptr };
    const Texture* metallicMap { nullptr };
    const Texture* aoMap { nullptr };
    const Texture* normalMap { nullptr };
    bool visible { true };
    float roughness { 0.5f };
    float metallic { 0.0f };
    bool rim { true };
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
        , m_metalAlbedo(RESOURCE_ROOT "resources/painted_metal/PaintedMetal006_1K-JPG_Color.jpg", ColorSpace::Srgb)
        , m_metalRoughness(RESOURCE_ROOT "resources/painted_metal/PaintedMetal006_1K-JPG_Roughness.jpg", ColorSpace::Linear)
        , m_metalMetallic(RESOURCE_ROOT "resources/painted_metal/PaintedMetal006_1K-JPG_Metalness.jpg", ColorSpace::Linear)
        , m_metalAo(RESOURCE_ROOT "resources/painted_metal/PaintedMetal006_1K-JPG_AmbientOcclusion.jpg", ColorSpace::Linear)
        , m_metalNormal(RESOURCE_ROOT "resources/painted_metal/PaintedMetal006_1K-JPG_NormalGL.jpg", ColorSpace::Linear)
        , m_skyboxTexture({ RESOURCE_ROOT "resources/skybox/px.png", RESOURCE_ROOT "resources/skybox/nx.png",
                              RESOURCE_ROOT "resources/skybox/py.png", RESOURCE_ROOT "resources/skybox/ny.png",
                              RESOURCE_ROOT "resources/skybox/pz.png", RESOURCE_ROOT "resources/skybox/nz.png" },
              ColorSpace::Srgb)
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
        m_window.registerScrollCallback([this](const glm::vec2& offset) {
            if (m_cameraMode == 2 && !ImGui::GetIO().WantCaptureMouse)
                m_orbitCamera.zoom(offset.y);
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

            ShaderBuilder tonemapBuilder;
            tonemapBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/tonemap_vert.glsl");
            tonemapBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/tonemap_frag.glsl");
            m_tonemapShader = tonemapBuilder.build();

            ShaderBuilder bezierBuilder;
            bezierBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/bezier_vert.glsl");
            bezierBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/bezier_frag.glsl");
            m_bezierShader = bezierBuilder.build();

            ShaderBuilder skyboxBuilder;
            skyboxBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/skybox_vert.glsl");
            skyboxBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/skybox_frag.glsl");
            m_skyboxShader = skyboxBuilder.build();
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

            // move the cart along the path
            const glm::vec3 cartPosition = m_path.position(m_time * m_cartSpeed);
            m_objects[m_cartIndex].modelMatrix = glm::scale(glm::translate(glm::mat4(1.0f), cartPosition), glm::vec3(0.15f));
            m_objects[m_cartIndex].visible = m_showPath;

            // camera (ignore mouse while over the UI)
            m_camera.setUserInteraction(m_cameraMode == 0 && !ImGui::GetIO().WantCaptureMouse);
            m_camera.updateInput(m_deltaTime);
            m_orbitCamera.updateInput(m_window, m_cameraMode == 2 && !ImGui::GetIO().WantCaptureMouse);

            // view and projection
            const glm::ivec2 framebufferSize = m_window.getFrameBufferSize();
            const float aspectRatio = framebufferSize.y > 0 ? static_cast<float>(framebufferSize.x) / static_cast<float>(framebufferSize.y) : 1.0f;
            glm::mat4 viewMatrix = m_camera.viewMatrix();
            glm::mat4 projectionMatrix = glm::perspective(glm::radians(60.0f), aspectRatio, 0.1f, 50.0f);
            if (m_cameraMode == 1) {
                viewMatrix = m_topCamera.viewMatrix();
                projectionMatrix = m_topCamera.projectionMatrix(aspectRatio);
            }
            if (m_cameraMode == 2)
                viewMatrix = m_orbitCamera.viewMatrix();
            if (m_cameraMode == 3)
                viewMatrix = m_chaseCamera.viewMatrix(cartPosition);

            // render into the HDR framebuffer
            m_hdrFramebuffer.resize(framebufferSize);
            m_hdrFramebuffer.bind();
            glViewport(0, 0, framebufferSize.x, framebufferSize.y);
            const glm::vec3 background = srgb(0.2f, 0.2f, 0.2f);
            glClearColor(background.r, background.g, background.b, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            const float noNormal[] = { 0.0f, 0.0f, 0.0f, 0.0f };
            glClearBufferfv(GL_COLOR, 1, noNormal);
            glEnable(GL_DEPTH_TEST);

            // draw objects
            m_defaultShader.bind();

            // light
            glUniform3fv(m_defaultShader.getUniformLocation("lightDirection"), 1, glm::value_ptr(m_lightDirection));
            glUniform3fv(m_defaultShader.getUniformLocation("lightColor"), 1, glm::value_ptr(m_lightColor));
            glUniform1f(m_defaultShader.getUniformLocation("lightIntensity"), m_lightIntensity);
            glUniform1f(m_defaultShader.getUniformLocation("ambientStrength"), m_ambientStrength);

            // camera position from the inverse view matrix
            const glm::vec3 cameraPosition = glm::vec3(glm::inverse(viewMatrix)[3]);
            glUniform3fv(m_defaultShader.getUniformLocation("cameraPosition"), 1, glm::value_ptr(cameraPosition));

            // point lights (a disabled light sends no light)
            for (size_t i = 0; i < m_pointLights.size(); ++i) {
                const PointLight& light = m_pointLights[i];
                const glm::vec3 radiance = light.enabled ? light.color * light.intensity : glm::vec3(0.0f);
                const std::string index = "[" + std::to_string(i) + "]";
                glUniform3fv(m_defaultShader.getUniformLocation("pointLightPositions" + index), 1, glm::value_ptr(light.position));
                glUniform3fv(m_defaultShader.getUniformLocation("pointLightRadiances" + index), 1, glm::value_ptr(radiance));
            }
            glUniform1i(m_defaultShader.getUniformLocation("unlit"), false);
            glUniform1i(m_defaultShader.getUniformLocation("shadingModel"), m_shadingModel);
            glUniform1i(m_defaultShader.getUniformLocation("celBands"), m_celBands);
            glUniform3fv(m_defaultShader.getUniformLocation("shadowTint"), 1, glm::value_ptr(m_shadowTint));
            glUniform1f(m_defaultShader.getUniformLocation("specularThreshold"), m_specularThreshold);
            glUniform1f(m_defaultShader.getUniformLocation("specularStrength"), m_specularStrength);
            glUniform3fv(m_defaultShader.getUniformLocation("rimColor"), 1, glm::value_ptr(m_rimColor));
            glUniform1f(m_defaultShader.getUniformLocation("rimSize"), m_rimSize);

            // environment
            glUniform1i(m_defaultShader.getUniformLocation("useEnvironment"), m_useReflections);
            m_skyboxTexture.bind(GL_TEXTURE5);
            glUniform1i(m_defaultShader.getUniformLocation("environmentMap"), 5);
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
                glUniform1f(m_defaultShader.getUniformLocation("baseRoughness"), object.roughness);
                glUniform1f(m_defaultShader.getUniformLocation("baseMetallic"), object.metallic);
                glUniform1i(m_defaultShader.getUniformLocation("hasRim"), object.rim && m_useRim);
                const bool useTexture = object.albedoMap && m_useAlbedoMaps;
                glUniform1i(m_defaultShader.getUniformLocation("hasAlbedoMap"), useTexture);
                if (useTexture) {
                    object.albedoMap->bind(GL_TEXTURE0);
                    glUniform1i(m_defaultShader.getUniformLocation("albedoMap"), 0);
                }
                const bool useRoughnessMap = object.roughnessMap && m_useRoughnessMaps;
                glUniform1i(m_defaultShader.getUniformLocation("hasRoughnessMap"), useRoughnessMap);
                if (useRoughnessMap) {
                    object.roughnessMap->bind(GL_TEXTURE1);
                    glUniform1i(m_defaultShader.getUniformLocation("roughnessMap"), 1);
                }
                const bool useMetallicMap = object.metallicMap && m_useMetallicMaps;
                glUniform1i(m_defaultShader.getUniformLocation("hasMetallicMap"), useMetallicMap);
                if (useMetallicMap) {
                    object.metallicMap->bind(GL_TEXTURE2);
                    glUniform1i(m_defaultShader.getUniformLocation("metallicMap"), 2);
                }
                const bool useAoMap = object.aoMap && m_useAoMaps;
                glUniform1i(m_defaultShader.getUniformLocation("hasAoMap"), useAoMap);
                if (useAoMap) {
                    object.aoMap->bind(GL_TEXTURE3);
                    glUniform1i(m_defaultShader.getUniformLocation("aoMap"), 3);
                }
                const bool useNormalMap = object.normalMap && m_useNormalMaps;
                glUniform1i(m_defaultShader.getUniformLocation("hasNormalMap"), useNormalMap);
                if (useNormalMap) {
                    object.normalMap->bind(GL_TEXTURE4);
                    glUniform1i(m_defaultShader.getUniformLocation("normalMap"), 4);
                    glUniform1f(m_defaultShader.getUniformLocation("normalStrength"), m_normalStrength);
                }

                for (const GPUMesh& mesh : *object.meshes)
                    mesh.draw();
            }

            // draw a small unlit sphere at each point light
            glUniform1i(m_defaultShader.getUniformLocation("unlit"), true);
            glUniform1i(m_defaultShader.getUniformLocation("hasAlbedoMap"), false);
            glUniform1i(m_defaultShader.getUniformLocation("hasRoughnessMap"), false);
            glUniform1i(m_defaultShader.getUniformLocation("hasMetallicMap"), false);
            for (const PointLight& light : m_pointLights) {
                if (!light.enabled)
                    continue;
                const glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), light.position), glm::vec3(0.08f));
                const glm::mat4 mvpMatrix = projectionMatrix * viewMatrix * model;
                glUniformMatrix4fv(m_defaultShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
                glUniformMatrix4fv(m_defaultShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(model));
                glUniform3fv(m_defaultShader.getUniformLocation("baseColor"), 1, glm::value_ptr(light.color));
                for (const GPUMesh& mesh : m_sphereMesh)
                    mesh.draw();
            }

            // draw the skybox (camera rotation only, always perspective)
            if (m_showSkybox) {
                const glm::mat4 skyProjection = glm::perspective(glm::radians(60.0f), aspectRatio, 0.1f, 50.0f);
                const glm::mat4 skyViewProjection = skyProjection * glm::mat4(glm::mat3(viewMatrix));
                glDepthFunc(GL_LEQUAL);
                m_skyboxShader.bind();
                glUniformMatrix4fv(m_skyboxShader.getUniformLocation("viewProjection"), 1, GL_FALSE, glm::value_ptr(skyViewProjection));
                m_skyboxTexture.bind(GL_TEXTURE5);
                glUniform1i(m_skyboxShader.getUniformLocation("skybox"), 5);
                for (const GPUMesh& mesh : m_cubeMesh)
                    mesh.draw();
                glDepthFunc(GL_LESS);
            }

            // draw the path
            if (m_showPath) {
                m_bezierShader.bind();
                const glm::mat4 pathMvp = projectionMatrix * viewMatrix;
                glUniformMatrix4fv(m_bezierShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(pathMvp));
                glUniform3f(m_bezierShader.getUniformLocation("color"), 1.0f, 0.4f, 0.1f);
                m_path.draw();
            }

            // tonemap to the screen
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDisable(GL_DEPTH_TEST);
            m_tonemapShader.bind();
            m_hdrFramebuffer.bindColorTexture(GL_TEXTURE0);
            glUniform1i(m_tonemapShader.getUniformLocation("hdrColor"), 0);
            glUniform1f(m_tonemapShader.getUniformLocation("exposure"), m_exposure);
            glUniform1i(m_tonemapShader.getUniformLocation("useTonemapping"), m_useTonemapping);
            m_hdrFramebuffer.bindDepthTexture(GL_TEXTURE1);
            glUniform1i(m_tonemapShader.getUniformLocation("depthTexture"), 1);
            glUniform1i(m_tonemapShader.getUniformLocation("useOutlines"), m_useOutlines);
            m_hdrFramebuffer.bindNormalTexture(GL_TEXTURE2);
            glUniform1i(m_tonemapShader.getUniformLocation("normalTexture"), 2);
            glUniform1f(m_tonemapShader.getUniformLocation("normalThreshold"), m_normalThreshold);
            glUniform1f(m_tonemapShader.getUniformLocation("outlineThickness"), m_outlineThickness);
            glUniform3fv(m_tonemapShader.getUniformLocation("inkColor"), 1, glm::value_ptr(m_inkColor));
            glUniform1i(m_tonemapShader.getUniformLocation("showEdgeView"), m_showEdgeView);
            glUniform1f(m_tonemapShader.getUniformLocation("depthThreshold"), m_depthThreshold);
            glUniform1f(m_tonemapShader.getUniformLocation("nearPlane"), 0.1f);
            glUniform1f(m_tonemapShader.getUniformLocation("farPlane"), m_cameraMode == 1 ? m_topCamera.height + 5.0f : 50.0f);
            glUniform1i(m_tonemapShader.getUniformLocation("orthographic"), m_cameraMode == 1);
            m_hdrFramebuffer.drawFullscreenTriangle();

            m_window.swapBuffers();
        }
    }

    void onKeyPressed(int key, int /* mods */)
    {
        // switch camera
        switch (key) {
        case GLFW_KEY_1:
            m_cameraMode = 0;
            break;
        case GLFW_KEY_2:
            m_cameraMode = 1;
            break;
        case GLFW_KEY_3:
            m_cameraMode = 2;
            break;
        case GLFW_KEY_4:
            m_cameraMode = 3;
            break;
        }
    }

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

        // orbit around the dragon's middle
        m_orbitCamera.target = glm::vec3(0.0f, 0.35f, 0.0f);

        // ground and dragon
        m_objects.push_back({ "Ground", &m_planeMesh, glm::scale(glm::mat4(1.0f), glm::vec3(6.0f, 1.0f, 6.0f)), srgb(0.45f, 0.47f, 0.5f) });
        m_objects.back().roughness = 0.8f;
        m_objects.back().rim = false;
        m_objects.push_back({ "Dragon", &m_dragonMesh, glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.283f, 0.0f)), srgb(1.0f, 0.78f, 0.34f) });
        m_objects.back().roughness = 0.3f;
        m_objects.back().metallic = 1.0f;

        // spheres
        const glm::vec3 sphereColors[] = { srgb(0.8f, 0.2f, 0.2f), srgb(0.2f, 0.7f, 0.3f), srgb(0.2f, 0.4f, 0.85f), srgb(0.9f, 0.8f, 0.2f) };
        for (int i = 0; i < 4; ++i) {
            const glm::vec3 position(1.3f, 0.3f, -1.2f + 0.8f * static_cast<float>(i));
            const glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), position), glm::vec3(0.6f));
            m_objects.push_back({ "Sphere " + std::to_string(i + 1), &m_sphereMesh, model, sphereColors[i] });
            m_objects.back().roughness = 0.15f + 0.25f * static_cast<float>(i);
        }

        // cube
        glm::mat4 cubeModel = glm::translate(glm::mat4(1.0f), glm::vec3(-1.3f, 0.35f, -0.6f));
        cubeModel = glm::rotate(cubeModel, glm::radians(30.0f), glm::vec3(0, 1, 0));
        cubeModel = glm::scale(cubeModel, glm::vec3(0.7f));
        m_objects.push_back({ "Cube", &m_cubeMesh, cubeModel, glm::vec3(1.0f), &m_metalAlbedo });
        m_objects.back().roughnessMap = &m_metalRoughness;
        m_objects.back().metallicMap = &m_metalMetallic;
        m_objects.back().aoMap = &m_metalAo;
        m_objects.back().normalMap = &m_metalNormal;

        // cart riding the path
        m_objects.push_back({ "Cart", &m_sphereMesh, glm::mat4(1.0f), srgb(0.95f, 0.95f, 0.95f) });
        m_cartIndex = m_objects.size() - 1;

        // cylinder
        const glm::mat4 cylinderModel = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-1.3f, 0.4f, 0.8f)), glm::vec3(0.5f, 0.8f, 0.5f));
        m_objects.push_back({ "Cylinder", &m_cylinderMesh, cylinderModel, srgb(0.6f, 0.35f, 0.75f) });
    }

    void drawUI()
    {
        ImGui::Begin("Settings");
        ImGui::Text("%.1f FPS (%.2f ms)", static_cast<double>(ImGui::GetIO().Framerate), static_cast<double>(1000.0f / ImGui::GetIO().Framerate));
        ImGui::TextUnformatted("Camera: WASD move, R/F up/down, hold left mouse to look.");
        ImGui::Checkbox("Albedo maps", &m_useAlbedoMaps);
        ImGui::Checkbox("Roughness maps", &m_useRoughnessMaps);
        ImGui::Checkbox("Metallic maps", &m_useMetallicMaps);
        ImGui::Checkbox("AO maps", &m_useAoMaps);
        ImGui::Checkbox("Normal maps", &m_useNormalMaps);
        ImGui::SliderFloat("Normal strength", &m_normalStrength, 0.0f, 3.0f);
        const char* shadingModels[] = { "Blinn-Phong", "PBR", "Cel" };
        ImGui::Combo("Shading", &m_shadingModel, shadingModels, 3);
        if (m_shadingModel == 2) {
            ImGui::SliderInt("Bands", &m_celBands, 2, 5);
            ImGui::ColorEdit3("Shadow tint", glm::value_ptr(m_shadowTint));
            ImGui::SliderFloat("Highlight threshold", &m_specularThreshold, 0.7f, 1.5f);
            ImGui::SliderFloat("Highlight strength", &m_specularStrength, 0.0f, 0.5f);
            ImGui::Checkbox("Rim light", &m_useRim);
            ImGui::ColorEdit3("Rim color", glm::value_ptr(m_rimColor));
            ImGui::SliderFloat("Rim size", &m_rimSize, 0.0f, 0.5f);
        }
        if (ImGui::CollapsingHeader("Camera")) {
            const char* cameraModes[] = { "1: Free", "2: Top", "3: Orbit", "4: Chase" };
            ImGui::Combo("Mode", &m_cameraMode, cameraModes, 4);
            if (m_cameraMode == 1)
                ImGui::SliderFloat("Top view size", &m_topCamera.viewSize, 1.0f, 10.0f);
            if (m_cameraMode == 2)
                ImGui::TextUnformatted("Drag to rotate, scroll to zoom.");
        }
        if (ImGui::CollapsingHeader("Bezier path")) {
            ImGui::Checkbox("Show path and cart", &m_showPath);
            ImGui::SliderFloat("Cart speed", &m_cartSpeed, 0.0f, 2.0f, "%.2f segments/s");
        }
        if (ImGui::CollapsingHeader("Time")) {
            ImGui::Text("Time: %.2f s", static_cast<double>(m_time));
            ImGui::Checkbox("Pause", &m_paused);
            if (ImGui::Checkbox("VSync", &m_vsync))
                glfwSwapInterval(m_vsync ? 1 : 0);
        }
        if (ImGui::CollapsingHeader("Environment")) {
            ImGui::Checkbox("Skybox", &m_showSkybox);
            ImGui::Checkbox("Reflections", &m_useReflections);
        }
        if (ImGui::CollapsingHeader("Light")) {
            ImGui::SliderFloat3("Direction", glm::value_ptr(m_lightDirection), -1.0f, 1.0f);
            ImGui::ColorEdit3("Color", glm::value_ptr(m_lightColor));
            ImGui::SliderFloat("Intensity", &m_lightIntensity, 0.0f, 10.0f);
            ImGui::SliderFloat("Ambient", &m_ambientStrength, 0.0f, 0.5f);
        }
        if (ImGui::CollapsingHeader("Point lights")) {
            for (size_t i = 0; i < m_pointLights.size(); ++i) {
                PointLight& light = m_pointLights[i];
                ImGui::PushID(static_cast<int>(i));
                ImGui::Text("Point light %d", static_cast<int>(i + 1));
                ImGui::Checkbox("On", &light.enabled);
                ImGui::SliderFloat3("Position", glm::value_ptr(light.position), -3.0f, 3.0f);
                ImGui::ColorEdit3("Color", glm::value_ptr(light.color));
                ImGui::SliderFloat("Intensity", &light.intensity, 0.0f, 10.0f);
                ImGui::PopID();
            }
        }
        if (ImGui::CollapsingHeader("Outlines")) {
            ImGui::Checkbox("Show outlines", &m_useOutlines);
            ImGui::SliderFloat("Depth threshold", &m_depthThreshold, 0.05f, 2.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
            ImGui::SliderFloat("Normal threshold", &m_normalThreshold, 0.1f, 4.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
            ImGui::SliderFloat("Thickness", &m_outlineThickness, 1.0f, 3.0f);
            ImGui::ColorEdit3("Ink color", glm::value_ptr(m_inkColor));
            ImGui::Checkbox("Debug: show edges only", &m_showEdgeView);
        }
        if (ImGui::CollapsingHeader("HDR")) {
            ImGui::SliderFloat("Exposure", &m_exposure, 0.1f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
            ImGui::Checkbox("Tonemapping (Reinhard)", &m_useTonemapping);
        }
        if (ImGui::CollapsingHeader("Materials")) {
            for (size_t i = 0; i < m_objects.size(); ++i) {
                ImGui::PushID(static_cast<int>(i));
                if (ImGui::TreeNode(m_objects[i].name.c_str())) {
                    ImGui::SliderFloat("Roughness", &m_objects[i].roughness, 0.05f, 1.0f);
                    ImGui::SliderFloat("Metallic", &m_objects[i].metallic, 0.0f, 1.0f);
                    ImGui::Checkbox("Rim light", &m_objects[i].rim);
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
        }
        if (ImGui::CollapsingHeader("Objects")) {
            for (size_t i = 0; i < m_objects.size(); ++i) {
                if (i != m_cartIndex)
                    ImGui::Checkbox(m_objects[i].name.c_str(), &m_objects[i].visible);
            }
        }
        ImGui::End();
    }

    Window m_window;
    Camera m_camera;
    TopCamera m_topCamera;
    OrbitCamera m_orbitCamera;
    ChaseCamera m_chaseCamera;
    int m_cameraMode { 0 };

    Shader m_defaultShader;
    Shader m_shadowShader;
    Shader m_tonemapShader;
    Shader m_bezierShader;
    Shader m_skyboxShader;

    HdrFramebuffer m_hdrFramebuffer;

    std::vector<GPUMesh> m_planeMesh, m_cubeMesh, m_sphereMesh, m_cylinderMesh, m_dragonMesh;
    Texture m_metalAlbedo;
    Texture m_metalRoughness;
    Texture m_metalMetallic;
    Texture m_metalAo;
    Texture m_metalNormal;
    Texture m_skyboxTexture;
    bool m_showSkybox { true };
    bool m_useReflections { true };
    std::vector<SceneObject> m_objects;

    BezierPath m_path;
    size_t m_cartIndex { 0 };
    float m_cartSpeed { 0.3f };
    bool m_showPath { true };

    bool m_useAlbedoMaps { true };
    bool m_useRoughnessMaps { true };
    bool m_useMetallicMaps { true };
    bool m_useAoMaps { true };
    bool m_useNormalMaps { true };
    float m_normalStrength { 1.0f };
    int m_shadingModel { 2 };
    int m_celBands { 3 };
    glm::vec3 m_shadowTint { 0.10f, 0.12f, 0.28f };
    float m_specularThreshold { 1.0f };
    float m_specularStrength { 0.5f };
    bool m_useRim { true };
    glm::vec3 m_rimColor { 0.5f };
    float m_rimSize { 0.25f };
    float m_lastFrameTime { 0.0f };
    float m_deltaTime { 0.0f };
    float m_time { 0.0f };
    bool m_paused { false };
    bool m_vsync { true };
    float m_exposure { 1.0f };
    bool m_useTonemapping { false };
    bool m_useOutlines { true };
    float m_depthThreshold { 0.3f };
    float m_normalThreshold { 1.0f };
    float m_outlineThickness { 1.0f };
    glm::vec3 m_inkColor { 0.0f };
    bool m_showEdgeView { false };

    glm::vec3 m_lightDirection { 0.4f, 1.0f, 0.3f };
    glm::vec3 m_lightColor { 1.0f };
    float m_lightIntensity { 3.0f };
    float m_ambientStrength { 0.15f };

    std::vector<PointLight> m_pointLights {
        { glm::vec3(1.3f, 1.0f, 0.0f), glm::vec3(1.0f, 0.6f, 0.3f) },
        { glm::vec3(-1.3f, 1.0f, 0.0f), glm::vec3(0.3f, 0.6f, 1.0f) },
    };
};

int main()
{
    Application app;
    app.update();

    return 0;
}
