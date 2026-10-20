#version 410

uniform mat4 mvpMatrix;
uniform mat4 modelMatrix;
// Normals should be transformed differently than positions:
// https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html
uniform mat3 normalModelMatrix;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec4 tangent; // xyz: direction of increasing u, w: handedness (see mesh.cpp).

out vec3 fragPosition;
out vec3 fragNormal;
out vec2 fragTexCoord;
out vec4 fragTangent;

void main()
{
    gl_Position = mvpMatrix * vec4(position, 1);

    fragPosition = (modelMatrix * vec4(position, 1)).xyz;
    fragNormal   = normalModelMatrix * normal;
    fragTexCoord = texCoord;
    // A tangent lies along the surface, so it transforms with the model matrix itself (not the inverse transpose).
    fragTangent  = vec4(mat3(modelMatrix) * tangent.xyz, tangent.w);
}
