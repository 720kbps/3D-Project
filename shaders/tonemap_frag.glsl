#version 410

uniform sampler2D hdrColor;
uniform float exposure;
uniform bool useTonemapping;
uniform sampler2D depthTexture;
uniform bool useOutlines;
uniform float depthThreshold;
uniform sampler2D normalTexture;
uniform float normalThreshold;
uniform float nearPlane;
uniform float farPlane;
uniform bool orthographic;

in vec2 texCoord;

layout(location = 0) out vec4 fragColor;

// depth buffer value -> distance from the camera
float linearDepth(vec2 uv)
{
    float depth = texture(depthTexture, uv).r;
    if (orthographic)
        return nearPlane + depth * (farPlane - nearPlane);
    float z = depth * 2.0 - 1.0;
    return 2.0 * nearPlane * farPlane / (farPlane + nearPlane - z * (farPlane - nearPlane));
}

// Sobel: how fast the depth changes around this pixel
float depthEdge()
{
    vec2 texel = 1.0 / vec2(textureSize(depthTexture, 0));

    // 3x3 neighbourhood
    float d[9];
    for (int y = -1; y <= 1; ++y)
        for (int x = -1; x <= 1; ++x)
            d[(y + 1) * 3 + (x + 1)] = linearDepth(texCoord + vec2(x, y) * texel);

    // horizontal and vertical change
    float gx = (d[2] + 2.0 * d[5] + d[8]) - (d[0] + 2.0 * d[3] + d[6]);
    float gy = (d[6] + 2.0 * d[7] + d[8]) - (d[0] + 2.0 * d[1] + d[2]);

    // relative to the distance, so far objects get outlines too
    return sqrt(gx * gx + gy * gy) / d[4];
}

// Sobel: how fast the surface direction changes around this pixel
float normalEdge()
{
    vec2 texel = 1.0 / vec2(textureSize(normalTexture, 0));

    // 3x3 neighbourhood
    vec3 n[9];
    for (int y = -1; y <= 1; ++y)
        for (int x = -1; x <= 1; ++x)
            n[(y + 1) * 3 + (x + 1)] = texture(normalTexture, texCoord + vec2(x, y) * texel).xyz;

    // horizontal and vertical change
    vec3 gx = (n[2] + 2.0 * n[5] + n[8]) - (n[0] + 2.0 * n[3] + n[6]);
    vec3 gy = (n[6] + 2.0 * n[7] + n[8]) - (n[0] + 2.0 * n[1] + n[2]);
    return sqrt(dot(gx, gx) + dot(gy, gy));
}

void main()
{
    // exposure
    vec3 color = texture(hdrColor, texCoord).rgb * exposure;

    // outlines
    if (useOutlines && (depthEdge() > depthThreshold || normalEdge() > normalThreshold))
        color = vec3(0.0);

    // tonemap (Reinhard)
    if (useTonemapping)
        color = color / (1.0 + color);

    // gamma
    fragColor = vec4(pow(color, vec3(1.0 / 2.2)), 1.0);
}
