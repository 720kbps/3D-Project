#version 410

uniform samplerCube skybox;

in vec3 direction;

layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = vec4(texture(skybox, direction).rgb, 1.0);
}
