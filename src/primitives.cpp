#include "primitives.h"
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/gtc/constants.hpp>
DISABLE_WARNINGS_POP()
#include <cmath>
#include <cstdint>

namespace {
// The framework's Material leaves kd uninitialised; set it so no garbage is ever copied around.
// (Entities bring their own SurfaceMaterial, so this one is not used for drawing.)
Mesh emptyMesh()
{
    Mesh mesh;
    mesh.material.kd = glm::vec3(1.0f);
    return mesh;
}

uint32_t addVertex(Mesh& mesh, const glm::vec3& position, const glm::vec3& normal, const glm::vec2& texCoord)
{
    mesh.vertices.push_back(Vertex { position, normal, texCoord });
    return static_cast<uint32_t>(mesh.vertices.size() - 1);
}

// Two triangles a-b-c and a-c-d. List the corners counter-clockwise as seen from the front.
void addQuad(Mesh& mesh, uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    mesh.triangles.emplace_back(a, b, c);
    mesh.triangles.emplace_back(a, c, d);
}
}

Mesh makePlane(float uvRepeat)
{
    Mesh mesh = emptyMesh();
    const glm::vec3 up { 0, 1, 0 };
    // u runs along +X and v along -Z, so a texture reads upright when seen from above with -Z as "up".
    const uint32_t a = addVertex(mesh, { -0.5f, 0, -0.5f }, up, glm::vec2(0, 1) * uvRepeat);
    const uint32_t b = addVertex(mesh, { -0.5f, 0, +0.5f }, up, glm::vec2(0, 0) * uvRepeat);
    const uint32_t c = addVertex(mesh, { +0.5f, 0, +0.5f }, up, glm::vec2(1, 0) * uvRepeat);
    const uint32_t d = addVertex(mesh, { +0.5f, 0, -0.5f }, up, glm::vec2(1, 1) * uvRepeat);
    addQuad(mesh, a, b, c, d);
    return mesh;
}

Mesh makeCube()
{
    // Per face: outward normal n, and the face's texture axes u (right) and v (up), chosen so that
    // cross(u, v) == n. That makes the corner order below counter-clockwise seen from outside.
    struct Face {
        glm::vec3 n, u, v;
    };
    const Face faces[] = {
        { { +1, 0, 0 }, { 0, 0, -1 }, { 0, 1, 0 } },
        { { -1, 0, 0 }, { 0, 0, +1 }, { 0, 1, 0 } },
        { { 0, +1, 0 }, { 1, 0, 0 }, { 0, 0, -1 } },
        { { 0, -1, 0 }, { 1, 0, 0 }, { 0, 0, +1 } },
        { { 0, 0, +1 }, { +1, 0, 0 }, { 0, 1, 0 } },
        { { 0, 0, -1 }, { -1, 0, 0 }, { 0, 1, 0 } },
    };

    Mesh mesh = emptyMesh();
    for (const Face& f : faces) {
        const glm::vec3 centre = 0.5f * f.n;
        const uint32_t a = addVertex(mesh, centre - 0.5f * f.u - 0.5f * f.v, f.n, { 0, 0 });
        const uint32_t b = addVertex(mesh, centre + 0.5f * f.u - 0.5f * f.v, f.n, { 1, 0 });
        const uint32_t c = addVertex(mesh, centre + 0.5f * f.u + 0.5f * f.v, f.n, { 1, 1 });
        const uint32_t d = addVertex(mesh, centre - 0.5f * f.u + 0.5f * f.v, f.n, { 0, 1 });
        addQuad(mesh, a, b, c, d);
    }
    return mesh;
}

Mesh makeUVSphere(int slices, int stacks)
{
    Mesh mesh = emptyMesh();
    // Grid of (stacks + 1) rings from the north pole (theta = 0) to the south pole (theta = pi),
    // each with (slices + 1) vertices around the axis; the last column repeats the first with u = 1.
    for (int i = 0; i <= stacks; ++i) {
        const float theta = glm::pi<float>() * static_cast<float>(i) / static_cast<float>(stacks);
        for (int j = 0; j <= slices; ++j) {
            const float phi = glm::two_pi<float>() * static_cast<float>(j) / static_cast<float>(slices);
            // -sin(phi) on Z makes u increase to the right when the sphere is seen from outside (no mirrored textures).
            const glm::vec3 normal { std::sin(theta) * std::cos(phi), std::cos(theta), -std::sin(theta) * std::sin(phi) };
            const glm::vec2 uv { static_cast<float>(j) / static_cast<float>(slices), 1.0f - static_cast<float>(i) / static_cast<float>(stacks) };
            addVertex(mesh, 0.5f * normal, normal, uv);
        }
    }

    const auto index = [&](int i, int j) { return static_cast<uint32_t>(i * (slices + 1) + j); };
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            const uint32_t a = index(i, j), b = index(i + 1, j), c = index(i + 1, j + 1), d = index(i, j + 1);
            // At the poles one triangle of each quad has zero area (two corners are the pole); skip it.
            if (i != stacks - 1)
                mesh.triangles.emplace_back(a, b, c);
            if (i != 0)
                mesh.triangles.emplace_back(a, c, d);
        }
    }
    return mesh;
}

Mesh makeCylinder(int slices)
{
    Mesh mesh = emptyMesh();
    const auto direction = [&](int j) {
        const float phi = glm::two_pi<float>() * static_cast<float>(j) / static_cast<float>(slices);
        return glm::vec3(std::cos(phi), 0.0f, -std::sin(phi)); // Same orientation as the sphere.
    };

    // Side: a bottom and a top vertex per column, normals pointing straight out.
    const uint32_t sideStart = static_cast<uint32_t>(mesh.vertices.size());
    for (int j = 0; j <= slices; ++j) {
        const glm::vec3 dir = direction(j);
        const float u = static_cast<float>(j) / static_cast<float>(slices);
        addVertex(mesh, 0.5f * dir + glm::vec3(0, -0.5f, 0), dir, { u, 0 });
        addVertex(mesh, 0.5f * dir + glm::vec3(0, +0.5f, 0), dir, { u, 1 });
    }
    for (int j = 0; j < slices; ++j) {
        const uint32_t bottom = sideStart + static_cast<uint32_t>(2 * j);
        addQuad(mesh, bottom, bottom + 2, bottom + 3, bottom + 1);
    }

    // Caps: a fan around a centre vertex. The texture is projected straight down onto the cap.
    for (const float y : { +0.5f, -0.5f }) {
        const glm::vec3 normal { 0, y > 0 ? 1.0f : -1.0f, 0 };
        const uint32_t centre = addVertex(mesh, { 0, y, 0 }, normal, { 0.5f, 0.5f });
        const uint32_t ringStart = static_cast<uint32_t>(mesh.vertices.size());
        for (int j = 0; j < slices; ++j) {
            const glm::vec3 p = 0.5f * direction(j) + glm::vec3(0, y, 0);
            addVertex(mesh, p, normal, { 0.5f + p.x, y > 0 ? 0.5f - p.z : 0.5f + p.z });
        }
        for (int j = 0; j < slices; ++j) {
            const uint32_t current = ringStart + static_cast<uint32_t>(j);
            const uint32_t next = ringStart + static_cast<uint32_t>((j + 1) % slices);
            // The bottom cap faces down, so its fan goes the other way round.
            if (y > 0)
                mesh.triangles.emplace_back(centre, current, next);
            else
                mesh.triangles.emplace_back(centre, next, current);
        }
    }
    return mesh;
}
