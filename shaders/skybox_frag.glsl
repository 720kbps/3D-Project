#version 410

uniform samplerCube skybox;

in vec3 direction;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 outNormal;

void main()
{
    outNormal = vec4(0.0);
    fragColor = vec4(texture(skybox, direction).rgb, 1.0);
}
