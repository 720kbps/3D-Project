#include "tangents.h"
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
DISABLE_WARNINGS_POP()
#include <cmath>
#include <cstdint>

namespace {
// Any unit vector perpendicular to n. Crossing with the coordinate axis least aligned with n avoids
// crossing two (nearly) parallel vectors, which would give a (nearly) zero result.
glm::vec3 anyPerpendicular(const glm::vec3& n)
{
    const glm::vec3 axis = std::abs(n.x) < 0.9f ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
    return glm::normalize(glm::cross(axis, n));
}
}

std::vector<glm::vec4> computeTangents(const Mesh& mesh)
{
    const size_t count = mesh.vertices.size();
    std::vector<glm::vec3> tangentSums(count, glm::vec3(0.0f));
    std::vector<glm::vec3> bitangentSums(count, glm::vec3(0.0f));

    for (const glm::uvec3& triangle : mesh.triangles) {
        const Vertex& v0 = mesh.vertices[triangle.x];
        const Vertex& v1 = mesh.vertices[triangle.y];
        const Vertex& v2 = mesh.vertices[triangle.z];

        // Inside a triangle, position is a linear function of (u, v): every edge is a mix of the two
        // directions T (per unit of u) and B (per unit of v), weighted by how much u and v change along it:
        //   e1 = du1 * T + dv1 * B
        //   e2 = du2 * T + dv2 * B
        // Two equations, two unknown vectors; solve them by inverting the 2x2 matrix of UV deltas.
        const glm::vec3 e1 = v1.position - v0.position;
        const glm::vec3 e2 = v2.position - v0.position;
        const glm::vec2 d1 = v1.texCoord - v0.texCoord;
        const glm::vec2 d2 = v2.texCoord - v0.texCoord;
        const float det = d1.x * d2.y - d2.x * d1.y;
        // det is (twice) the triangle's signed area in UV space. Zero means the triangle has no UVs or all
        // three UVs lie on a line, so u and v do not define directions here: skip it instead of dividing by 0.
        if (std::abs(det) < 1e-12f)
            continue;
        const float inverseDet = 1.0f / det;
        const glm::vec3 tangent = (d2.y * e1 - d1.y * e2) * inverseDet;
        const glm::vec3 bitangent = (d1.x * e2 - d2.x * e1) * inverseDet;

        // A vertex is shared by several triangles: add up their directions and average them below,
        // so the frame varies smoothly across the surface like the normals do.
        for (const uint32_t index : { triangle.x, triangle.y, triangle.z }) {
            tangentSums[index] += tangent;
            bitangentSums[index] += bitangent;
        }
    }

    std::vector<glm::vec4> result(count);
    for (size_t i = 0; i < count; ++i) {
        const glm::vec3 normal = mesh.vertices[i].normal;
        const float normalLength = glm::length(normal);
        const glm::vec3 n = normalLength > 0.0f ? normal / normalLength : glm::vec3(0, 1, 0);

        // Gram-Schmidt: remove the part of the averaged tangent that points along the normal, so T lies
        // exactly in the surface plane. (The UV directions are only perpendicular to N on flat, unstretched UVs.)
        glm::vec3 t = tangentSums[i] - n * glm::dot(n, tangentSums[i]);
        const float tangentLengthSquared = glm::dot(t, t);
        t = tangentLengthSquared > 1e-12f ? t / std::sqrt(tangentLengthSquared) : anyPerpendicular(n);

        // Handedness: cross(N, T) is the bitangent of a non-mirrored texture. If the real bitangent points
        // the other way, the texture is mirrored here and the shader must flip B.
        const float handedness = glm::dot(glm::cross(n, t), bitangentSums[i]) < 0.0f ? -1.0f : 1.0f;
        result[i] = glm::vec4(t, handedness);
    }
    return result;
}
