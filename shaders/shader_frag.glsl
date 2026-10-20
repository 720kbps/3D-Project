#version 410

uniform vec3 baseColor;
uniform bool hasAlbedoMap;
uniform sampler2D albedoMap;

in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord;
in vec4 fragTangent;

layout(location = 0) out vec4 fragColor;

void main()
{
    vec3 normal = normalize(fragNormal);
    vec3 albedo = hasAlbedoMap ? texture(albedoMap, fragTexCoord).rgb : baseColor;

    // diffuse from a fixed light
    vec3 lightDirection = normalize(vec3(0.4, 1.0, 0.3));
    float diffuse = max(dot(normal, lightDirection), 0.0);
    vec3 color = albedo * (0.15 + 0.85 * diffuse);

    // gamma
    fragColor = vec4(pow(color, vec3(1.0 / 2.2)), 1.0);
}
