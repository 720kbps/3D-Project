// SCAFFOLDING, not a graded feature. Source: Mihai (Phase 1). Plane, cube, sphere, cylinder for
// building the scene and the robot. Add new shapes here if needed.

#pragma once
#include <framework/mesh.h>

// Basic shapes generated in code, with normals and texture coordinates. Each fits in the unit cube
// centred on the origin, so a Transform's scale is the object's size in world units.
// Triangles wind counter-clockwise seen from outside (OpenGL's default front face), so back-face
// culling can be turned on later without holes appearing.

// Flat square in the XZ plane, facing +Y. uvRepeat > 1 tiles a texture across it.
Mesh makePlane(float uvRepeat = 1.0f);
// 24 vertices (4 per face) instead of 8, so every face gets its own flat normal and its own [0, 1] UVs.
Mesh makeCube();
// Latitude/longitude sphere of radius 0.5. Seam and pole vertices are duplicated so UVs do not wrap backwards.
Mesh makeUVSphere(int slices = 48, int stacks = 24);
// Radius 0.5, height 1 along Y, with flat caps (separate vertices so the edge stays sharp).
Mesh makeCylinder(int slices = 32);
