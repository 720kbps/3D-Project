#version 410

uniform mat4 viewProjection;

layout(location = 0) in vec3 position;

out vec3 direction;

void main()
{
    direction = position;

    // z = w puts the skybox at the far plane, behind everything
    vec4 clipPosition = viewProjection * vec4(position, 1.0);
    gl_Position = clipPosition.xyww;
}
