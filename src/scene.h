#pragma once
#include "mesh.h"
#include "texture.h"
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
DISABLE_WARNINGS_POP()
#include <framework/shader.h>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

// Texture units of the main shader, fixed per purpose. Two samplers of different types (e.g. sampler2D
// and samplerCube) must never point at the same unit, or the draw call fails. Add new maps here.
namespace TextureUnit {
inline constexpr GLint Albedo = 0;
inline constexpr GLint DebugCubemap = 1;
}

// Position, orientation and size of an object. matrix() composes them as T * R * S: glm multiplies
// right to left, so a vertex is scaled first, then rotated, then moved into place.
struct Transform {
    glm::vec3 position { 0.0f };
    glm::quat rotation { 1.0f, 0.0f, 0.0f, 0.0f }; // Identity. glm's quat constructor takes w first.
    glm::vec3 scale { 1.0f };

    [[nodiscard]] glm::mat4 matrix() const;
};

// How a surface looks. Kept apart from the mesh so one mesh can be drawn with different looks
// (e.g. a row of spheres). Grows with the shading features (roughness, normal map, ...).
// Named SurfaceMaterial because the framework already has a global Material (used by its OBJ loader).
struct SurfaceMaterial {
    glm::vec3 baseColor { 0.8f };
    const Texture* albedoMap { nullptr }; // Replaces baseColor when set. Not owned.
};

// One thing in the scene: which meshes to draw, where, and how they look.
struct Entity {
    std::string name;
    Transform transform;
    const std::vector<GPUMesh>* meshes { nullptr }; // Owned by the Scene. A model file can contain several sub-meshes.
    SurfaceMaterial material;
    bool visible { true };
};

// Owns the GPU meshes and the list of entities that draw them.
class Scene {
public:
    // Store meshes; the returned pointer stays valid for the scene's lifetime, so several entities can share them.
    const std::vector<GPUMesh>* addMeshes(std::vector<GPUMesh>&& meshes);
    const std::vector<GPUMesh>* addMesh(const Mesh& cpuMesh);

    // The returned reference is only valid until the next addEntity (the vector may reallocate).
    Entity& addEntity(std::string name, const std::vector<GPUMesh>* meshes, const Transform& transform = {}, const SurfaceMaterial& material = {});
    // nullptr if there is no entity with that name.
    Entity* find(std::string_view name);

    std::vector<Entity>& entities() { return m_entities; }
    const std::vector<Entity>& entities() const { return m_entities; }

private:
    // unique_ptr so the mesh lists never move in memory when more are added.
    std::vector<std::unique_ptr<std::vector<GPUMesh>>> m_meshStorage;
    std::vector<Entity> m_entities;
};

// Draws every visible entity with the main shader: sets the per-entity uniforms
// (mvpMatrix, modelMatrix, normalModelMatrix, baseColor, hasAlbedoMap, albedoMap) and issues the draw calls.
// Per-frame uniforms (camera, debug view, lights later) are the caller's job, after binding the shader.
void drawScene(const Scene& scene, const Shader& shader, const glm::mat4& view, const glm::mat4& projection);
