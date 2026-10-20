#include "scene.h"
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
DISABLE_WARNINGS_POP()
#include <utility>

glm::mat4 Transform::matrix() const
{
    return glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(rotation) * glm::scale(glm::mat4(1.0f), scale);
}

const std::vector<GPUMesh>* Scene::addMeshes(std::vector<GPUMesh>&& meshes)
{
    m_meshStorage.push_back(std::make_unique<std::vector<GPUMesh>>(std::move(meshes)));
    return m_meshStorage.back().get();
}

const std::vector<GPUMesh>* Scene::addMesh(const Mesh& cpuMesh)
{
    std::vector<GPUMesh> meshes;
    meshes.emplace_back(cpuMesh);
    return addMeshes(std::move(meshes));
}

Entity& Scene::addEntity(std::string name, const std::vector<GPUMesh>* meshes, const Transform& transform, const SurfaceMaterial& material)
{
    return m_entities.emplace_back(Entity { std::move(name), transform, meshes, material, true });
}

Entity* Scene::find(std::string_view name)
{
    for (Entity& entity : m_entities) {
        if (entity.name == name)
            return &entity;
    }
    return nullptr;
}

void drawScene(const Scene& scene, const Shader& shader, const glm::mat4& view, const glm::mat4& projection)
{
    shader.bind();
    const glm::mat4 viewProjection = projection * view;
    for (const Entity& entity : scene.entities()) {
        if (!entity.visible || !entity.meshes)
            continue;

        const glm::mat4 modelMatrix = entity.transform.matrix();
        const glm::mat4 mvpMatrix = viewProjection * modelMatrix;
        // Normals need the inverse transpose of the model matrix: with non-uniform scaling the plain
        // matrix would tilt them, so they would no longer be perpendicular to the surface.
        const glm::mat3 normalModelMatrix = glm::inverseTranspose(glm::mat3(modelMatrix));
        glUniformMatrix4fv(shader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
        glUniformMatrix4fv(shader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
        glUniformMatrix3fv(shader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrix));

        const SurfaceMaterial& material = entity.material;
        glUniform3fv(shader.getUniformLocation("baseColor"), 1, glm::value_ptr(material.baseColor));
        glUniform1i(shader.getUniformLocation("hasAlbedoMap"), material.albedoMap ? GL_TRUE : GL_FALSE);
        if (material.albedoMap) {
            material.albedoMap->bind(GL_TEXTURE0 + TextureUnit::Albedo);
            glUniform1i(shader.getUniformLocation("albedoMap"), TextureUnit::Albedo);
        }

        for (const GPUMesh& mesh : *entity.meshes)
            mesh.draw();
    }
}
