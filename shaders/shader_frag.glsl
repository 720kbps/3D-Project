#version 410

// Per-entity material, set by drawScene() in src/scene.cpp (struct SurfaceMaterial in src/scene.h).
uniform vec3 baseColor;
uniform bool hasAlbedoMap;
uniform sampler2D albedoMap;

// Debug views, selected in the UI. Values must match enum class DebugView in src/settings.h.
const int DEBUG_NORMALS = 1;
const int DEBUG_TEXCOORDS = 2;
const int DEBUG_DEPTH = 3;
uniform int debugView;
uniform vec3 cameraPosition;
uniform float depthViewRange;
// Debug cubemap (resources/debug/cubemap): stores d * 0.5 + 0.5 in direction d, so sampling it with the
// normal must reproduce the Normals view exactly. Any wrong face order or flip shows up as a mismatch.
const int DEBUG_CUBEMAP = 4;
uniform samplerCube debugCubemap;
const int DEBUG_TANGENTS = 5;
const int DEBUG_TANGENT_SIGN = 6;

// Second render target of the HDR framebuffer (src/hdr_framebuffer.h): view-space normals for screen-space passes.
uniform mat4 viewMatrix;

in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord;
in vec4 fragTangent; // xyz: world-space tangent (interpolated, so not unit length), w: handedness.

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 outViewNormal;

void main()
{
    vec3 normal = normalize(fragNormal);
    // Written before any early return: an output a fragment shader never writes is undefined.
    // The view matrix has no scaling, so its 3x3 part rotates normals correctly. Alpha 1 marks "geometry here".
    outViewNormal = vec4(normalize(mat3(viewMatrix) * normal), 1.0);

    if (debugView == DEBUG_NORMALS) {
        // Normals are in [-1, 1]; remap so negative directions are visible instead of clipped to black.
        fragColor = vec4(normal * 0.5 + 0.5, 1);
        return;
    }
    if (debugView == DEBUG_TEXCOORDS) {
        // fract() so tiled UVs (outside [0, 1]) show as repeating gradients.
        fragColor = vec4(fract(fragTexCoord), 0, 1);
        return;
    }
    if (debugView == DEBUG_DEPTH) {
        // Distance in world units, independent of the projection's near/far planes.
        float d = clamp(distance(fragPosition, cameraPosition) / depthViewRange, 0.0, 1.0);
        fragColor = vec4(vec3(1.0 - d), 1);
        return;
    }
    if (debugView == DEBUG_CUBEMAP) {
        fragColor = vec4(texture(debugCubemap, normal).rgb, 1);
        return;
    }
    if (debugView == DEBUG_TANGENTS) {
        fragColor = vec4(normalize(fragTangent.xyz) * 0.5 + 0.5, 1);
        return;
    }
    if (debugView == DEBUG_TANGENT_SIGN) {
        // w is +1 or -1 per vertex; inside a triangle it is interpolated, so test the sign.
        fragColor = fragTangent.w >= 0.0 ? vec4(0, 1, 0, 1) : vec4(1, 0, 0, 1);
        return;
    }

    // No lighting yet: flat colour. Shading (PBR) replaces this.
    vec3 albedo = hasAlbedoMap ? texture(albedoMap, fragTexCoord).rgb : baseColor;
    fragColor = vec4(albedo, 1);
}
