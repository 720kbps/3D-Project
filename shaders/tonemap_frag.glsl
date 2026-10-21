#version 410

uniform sampler2D hdrColor;
uniform float exposure;
uniform bool useTonemapping;

in vec2 texCoord;

layout(location = 0) out vec4 fragColor;

void main()
{
    // exposure
    vec3 color = texture(hdrColor, texCoord).rgb * exposure;

    // tonemap (Reinhard)
    if (useTonemapping)
        color = color / (1.0 + color);

    // gamma
    fragColor = vec4(pow(color, vec3(1.0 / 2.2)), 1.0);
}
