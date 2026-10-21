#version 410

uniform vec3 baseColor;
uniform bool hasAlbedoMap;
uniform sampler2D albedoMap;
uniform float roughness;
uniform float metallic;

uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform float lightIntensity;
uniform float ambientStrength;
uniform vec3 cameraPosition;

in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord;
in vec4 fragTangent;

layout(location = 0) out vec4 fragColor;

const float PI = 3.14159265;

// D: how many microfacets face the half vector (GGX)
float distributionGGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float d = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d);
}

// G: how many microfacets are not hidden by others (Smith, Schlick-GGX)
float geometrySchlickGGX(float NdotX, float roughness)
{
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    return NdotX / (NdotX * (1.0 - k) + k);
}

// F: how much light is reflected instead of entering the surface (Schlick)
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

void main()
{
    vec3 albedo = hasAlbedoMap ? texture(albedoMap, fragTexCoord).rgb : baseColor;

    // directions
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(lightDirection);
    vec3 V = normalize(cameraPosition - fragPosition);
    vec3 H = normalize(L + V);
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0001);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);

    // incoming light
    vec3 radiance = lightColor * lightIntensity;

    // specular (Cook-Torrance)
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    float D = distributionGGX(NdotH, roughness);
    float G = geometrySchlickGGX(NdotV, roughness) * geometrySchlickGGX(NdotL, roughness);
    vec3 F = fresnelSchlick(HdotV, F0);
    vec3 specular = D * G * F / (4.0 * NdotV * NdotL + 0.0001);

    // diffuse (Lambert)
    vec3 kD = (1.0 - F) * (1.0 - metallic);
    vec3 diffuse = kD * albedo / PI;

    // ambient
    vec3 ambient = albedo * ambientStrength;

    vec3 color = (diffuse + specular) * radiance * NdotL + ambient;

    fragColor = vec4(color, 1.0);
}
