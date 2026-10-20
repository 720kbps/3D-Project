#version 410

// Per-object material, set in Application::update().
uniform vec3 baseColor;
uniform bool hasAlbedoMap;
uniform sampler2D albedoMap;

in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord;
in vec4 fragTangent; // xyz: world-space tangent, w: handedness. Unused until normal mapping.

layout(location = 0) out vec4 fragColor;

void main()
{
    vec3 normal = normalize(fragNormal);
    vec3 albedo = hasAlbedoMap ? texture(albedoMap, fragTexCoord).rgb : baseColor;

    // PLACEHOLDER so shapes are readable: one fixed light, diffuse only. Replaced by PBR (task 2.2).
    vec3 lightDirection = normalize(vec3(0.4, 1.0, 0.3));
    float diffuse = max(dot(normal, lightDirection), 0.0);
    vec3 color = albedo * (0.15 + 0.85 * diffuse);

    // Colours are linear here; the screen expects sRGB. Approximate gamma 2.2 until the HDR/tonemap pass returns.
    fragColor = vec4(pow(color, vec3(1.0 / 2.2)), 1.0);
}
