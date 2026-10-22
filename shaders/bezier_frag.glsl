#version 410

uniform vec3 color;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 outNormal;

void main()
{
    outNormal = vec4(0.0);
    fragColor = vec4(color, 1.0);
}
