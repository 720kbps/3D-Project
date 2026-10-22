#version 410

uniform sampler2D hdrColor;
uniform float exposure;
uniform bool useTonemapping;
uniform sampler2D depthTexture;
uniform bool useOutlines;

in vec2 texCoord;

layout(location = 0) out vec4 fragColor;

// Sobel: how fast the depth changes around this pixel
float depthEdge()
{
    vec2 texel = 1.0 / vec2(textureSize(depthTexture, 0));

    // 3x3 neighbourhood
    float d[9];
    for (int y = -1; y <= 1; ++y)
        for (int x = -1; x <= 1; ++x)
            d[(y + 1) * 3 + (x + 1)] = texture(depthTexture, texCoord + vec2(x, y) * texel).r;

    // horizontal and vertical change
    float gx = (d[2] + 2.0 * d[5] + d[8]) - (d[0] + 2.0 * d[3] + d[6]);
    float gy = (d[6] + 2.0 * d[7] + d[8]) - (d[0] + 2.0 * d[1] + d[2]);
    return sqrt(gx * gx + gy * gy);
}

void main()
{
    // exposure
    vec3 color = texture(hdrColor, texCoord).rgb * exposure;

    // outlines
    if (useOutlines && depthEdge() > 0.002)
        color = vec3(0.0);

    // tonemap (Reinhard)
    if (useTonemapping)
        color = color / (1.0 + color);

    // gamma
    fragColor = vec4(pow(color, vec3(1.0 / 2.2)), 1.0);
}
