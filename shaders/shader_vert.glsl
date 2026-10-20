#version 410

uniform mat4 mvpMatrix;
uniform mat4 modelMatrix;
uniform mat3 normalModelMatrix;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec4 tangent;

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
    fragTangent  = vec4(mat3(modelMatrix) * tangent.xyz, tangent.w);
}
