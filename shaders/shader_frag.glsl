#version 410

uniform vec3 baseColor;
uniform bool hasAlbedoMap;
uniform sampler2D albedoMap;
uniform float baseRoughness;
uniform bool hasRoughnessMap;
uniform sampler2D roughnessMap;
uniform float baseMetallic;
uniform bool hasMetallicMap;
uniform sampler2D metallicMap;
uniform bool hasAoMap;
uniform sampler2D aoMap;
uniform bool hasNormalMap;
uniform sampler2D normalMap;
uniform float normalStrength;
uniform bool unlit;
uniform int shadingModel;
uniform int celBands;
uniform vec3 shadowTint;
uniform float specularThreshold;
uniform float specularStrength;
uniform bool hasRim;
uniform vec3 rimColor;
uniform float rimSize;

uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform float lightIntensity;
uniform float ambientStrength;
uniform vec3 cameraPosition;
uniform bool useEnvironment;
uniform samplerCube environmentMap;

uniform vec3 pointLightPositions[2];
uniform vec3 pointLightRadiances[2];

uniform sampler2D shadowMap;
uniform mat4 lightMVP;
uniform bool showLightDepth;
uniform bool useShadows;
uniform float shadowBias;

in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord;
in vec4 fragTangent;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 outNormal;

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

// light reflected toward the camera from one light
vec3 shadeLight(vec3 N, vec3 V, vec3 L, vec3 radiance, vec3 albedo, float roughness, float metallic)
{
    vec3 H = normalize(L + V);
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0001);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);

    // Blinn-Phong, with shininess from roughness
    if (shadingModel == 0) {
        float alpha = roughness * roughness;
        float shininess = 2.0 / (alpha * alpha) - 2.0;
        vec3 specular = vec3(0.25 * pow(NdotH, shininess));
        return (albedo / PI + specular) * radiance * NdotL;
    }

    // specular (Cook-Torrance)
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    float D = distributionGGX(NdotH, roughness);
    float G = geometrySchlickGGX(NdotV, roughness) * geometrySchlickGGX(NdotL, roughness);
    vec3 F = fresnelSchlick(HdotV, F0);
    vec3 specular = D * G * F / (4.0 * NdotV * NdotL + 0.0001);

    // diffuse (Lambert)
    vec3 kD = (1.0 - F) * (1.0 - metallic);
    vec3 diffuse = kD * albedo / PI;

    // cel: snap the diffuse light to flat bands
    float diffuseLight = NdotL;
    if (shadingModel == 2) {
        float x = NdotL * float(celBands);

        // smooth each band edge over about one pixel
        float edgeWidth = fwidth(x);
        float banded = floor(x) + smoothstep(1.0 - edgeWidth, 1.0, fract(x));

        diffuseLight = min(banded / float(celBands - 1), 1.0);

        // hard highlight: on where GGX is above a threshold
        float highlight = D * NdotL;
        float highlightWidth = fwidth(highlight);
        float mask = smoothstep(specularThreshold - highlightWidth, specularThreshold + highlightWidth, highlight);
        vec3 highlightColor = mix(vec3(1.0), albedo, metallic) * specularStrength;

        return diffuse * radiance * diffuseLight + highlightColor * mask * radiance;
    }

    return diffuse * radiance * diffuseLight + specular * radiance * NdotL;
}

// where this point lands in the sun's shadow map: xy = texture coordinate, z = depth
vec3 shadowMapPosition()
{
    vec4 fragLightCoord = lightMVP * vec4(fragPosition, 1.0);
    fragLightCoord.xyz /= fragLightCoord.w;
    return fragLightCoord.xyz * 0.5 + 0.5;
}

// 1 if the sun reaches this point, 0 if something closer to the sun blocks it
float sunVisibility()
{
    vec3 p = shadowMapPosition();

    // outside the shadow map: nothing blocks the sun
    if (p.x < 0.0 || p.x > 1.0 || p.y < 0.0 || p.y > 1.0 || p.z > 1.0)
        return 1.0;

    // shadow test with a small bias against self-shadowing
    float shadowMapDepth = texture(shadowMap, p.xy).x;
    return p.z - shadowBias > shadowMapDepth ? 0.0 : 1.0;
}

void main()
{
    // surface normal for the outline pass (without the normal map)
    outNormal = vec4(normalize(fragNormal), 1.0);

    // material
    vec3 albedo = hasAlbedoMap ? texture(albedoMap, fragTexCoord).rgb : baseColor;
    float roughness = hasRoughnessMap ? texture(roughnessMap, fragTexCoord).r : baseRoughness;
    roughness = max(roughness, 0.05);
    float metallic = hasMetallicMap ? texture(metallicMap, fragTexCoord).r : baseMetallic;
    float ao = hasAoMap ? texture(aoMap, fragTexCoord).r : 1.0;
    if (unlit) {
        fragColor = vec4(albedo, 1.0);
        return;
    }

    // debug: the depth the sun sees at this point, as grey
    if (showLightDepth) {
        float shadowMapDepth = texture(shadowMap, shadowMapPosition().xy).x;
        fragColor = vec4(vec3(shadowMapDepth), 1.0);
        return;
    }

    vec3 N = normalize(fragNormal);

    // normal map: tangent space -> world space
    if (hasNormalMap) {
        vec3 T = normalize(fragTangent.xyz - dot(fragTangent.xyz, N) * N);
        vec3 B = cross(N, T) * fragTangent.w;
        mat3 TBN = mat3(T, B, N);
        vec3 mapNormal = texture(normalMap, fragTexCoord).rgb * 2.0 - 1.0;
        mapNormal.xy *= normalStrength;
        N = normalize(TBN * mapNormal);
    }
    vec3 V = normalize(cameraPosition - fragPosition);

    // sun, switched off where it is in shadow
    float sunLight = useShadows ? sunVisibility() : 1.0;
    vec3 color = shadeLight(N, V, normalize(lightDirection), lightColor * lightIntensity * sunLight, albedo, roughness, metallic);

    // point lights (inverse-square falloff)
    for (int i = 0; i < 2; ++i) {
        vec3 toLight = pointLightPositions[i] - fragPosition;
        float distanceSquared = dot(toLight, toLight);
        color += shadeLight(N, V, normalize(toLight), pointLightRadiances[i] / distanceSquared, albedo, roughness, metallic);
    }

    // ambient (cel: coloured shadow tint), darkened in crevices
    if (shadingModel == 2)
        color += albedo * shadowTint * ao;
    else
        color += albedo * ambientStrength * ao;

    // cel: rim light on the lit side of the silhouette
    if (shadingModel == 2 && hasRim) {
        float rim = (1.0 - max(dot(N, V), 0.0)) * pow(max(dot(N, normalize(lightDirection)), 0.0), 0.1);
        float rimWidth = fwidth(rim);
        float rimMask = smoothstep(1.0 - rimSize - rimWidth, 1.0 - rimSize + rimWidth, rim);
        color += rimColor * rimMask;
    }

    // environment reflection
    if (useEnvironment) {
        vec3 R = reflect(-V, N);
        vec3 F0 = mix(vec3(0.04), albedo, metallic);
        vec3 F = fresnelSchlick(max(dot(N, V), 0.0), F0);

        // rougher surface -> blurrier mip level
        float maxMip = log2(float(textureSize(environmentMap, 0).x));
        vec3 environment = textureLod(environmentMap, R, roughness * maxMip).rgb;
        color += environment * F * ao;
    }

    fragColor = vec4(color, 1.0);
}
