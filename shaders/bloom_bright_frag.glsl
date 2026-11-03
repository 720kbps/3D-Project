#version 410

uniform sampler2D hdrColor;
uniform float threshold;

in vec2 texCoord;

layout(location = 0) out vec4 fragColor;

void main()
{
    vec3 color = texture(hdrColor, texCoord).rgb;

    // keep only the part above the threshold
    float brightness = max(color.r, max(color.g, color.b));
    float amount = max(brightness - threshold, 0.0) / max(brightness, 0.0001);

    fragColor = vec4(color * amount, 1.0);
}
