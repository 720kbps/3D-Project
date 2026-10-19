#pragma once
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/vec4.hpp>
DISABLE_WARNINGS_POP()
#include <framework/mesh.h>
#include <vector>

// Tangent frame per vertex, for normal mapping. Returns one value per vertex of `mesh`, in the same order.
//  xyz: unit tangent T. The surface direction in which the texture coordinate u increases, made exactly
//       perpendicular to the vertex normal.
//  w:   +1 or -1, the handedness. The bitangent (direction of increasing v) is cross(N, T) * w;
//       w is -1 where the texture is mirrored on the surface, so the shader can rebuild B correctly.
// Vertices that only belong to triangles without usable UVs get an arbitrary tangent perpendicular to N.
std::vector<glm::vec4> computeTangents(const Mesh& mesh);
