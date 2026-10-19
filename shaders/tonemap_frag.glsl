#version 410

// Final pass: turns the HDR image into colours the display can show. The order matters:
//   1. exposure   - scale the linear light, like a camera's shutter,
//   2. tonemap    - squeeze [0, infinity) into [0, 1) so highlights roll off instead of clipping,
//   3. sRGB encode - the display expects gamma-encoded values; everything before this step is linear.

uniform sampler2D hdrColor;
uniform sampler2D viewNormals;
uniform sampler2D depthBuffer;

uniform float exposure; // Linear multiplier (2^EV).
uniform int tonemapper; // Values match enum class Tonemapper in src/settings.h.
uniform bool srgbEncode;
uniform bool passthrough; // The main pass is showing a debug view: show its values unchanged.
uniform int bufferView; // Values match enum class BufferView in src/settings.h.
uniform vec2 depthLinearize; // (projection[2][2], projection[3][2]) of the camera that rendered the depth buffer.
uniform float depthViewRange;

in vec2 texCoord;
layout(location = 0) out vec4 fragColor;

const int TONEMAP_REINHARD = 1;
const int TONEMAP_ACES = 2;
const int BUFFER_NORMALS = 1;
const int BUFFER_DEPTH = 2;

vec3 reinhard(vec3 c)
{
    return c / (1.0 + c);
}

// Curve fit of the ACES filmic tone curve by Krzysztof Narkowicz, "ACES Filmic Tone Mapping Curve" (blog post, 2016).
vec3 acesFitted(vec3 c)
{
    return (c * (2.51 * c + 0.03)) / (c * (2.43 * c + 0.59) + 0.14);
}

// Exact sRGB encoding (IEC 61966-2-1): a short linear segment near black, a 1/2.4 power above it.
// Using the exact curve (not pow 1/2.2) means an sRGB texture shown untouched comes out bit-identical.
vec3 linearToSrgb(vec3 c)
{
    vec3 low = c * 12.92;
    vec3 high = 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055;
    return mix(low, high, step(vec3(0.0031308), c));
}

void main()
{
    if (bufferView == BUFFER_NORMALS) {
        vec4 n = texture(viewNormals, texCoord);
        fragColor = vec4(n.a > 0.0 ? n.xyz * 0.5 + 0.5 : vec3(0.0), 1.0);
        return;
    }
    if (bufferView == BUFFER_DEPTH) {
        // Undo the perspective depth mapping: for a glm perspective matrix, ndcZ = (A*z + B) / -z,
        // so the distance along the view axis is B / (ndcZ + A). Exact for any near/far planes.
        float ndcZ = texture(depthBuffer, texCoord).r * 2.0 - 1.0;
        float distance = depthLinearize.y / (ndcZ + depthLinearize.x);
        fragColor = vec4(vec3(1.0 - clamp(distance / depthViewRange, 0.0, 1.0)), 1.0);
        return;
    }

    vec3 color = texture(hdrColor, texCoord).rgb;
    if (passthrough) {
        // Debug values are shown as-is, but the background is still the linear clear colour, so encode
        // that as usual. The normal buffer's alpha tells geometry (1) from background (0).
        bool geometry = texture(viewNormals, texCoord).a > 0.0;
        fragColor = vec4(geometry ? color : linearToSrgb(color), 1.0);
        return;
    }

    color *= exposure;
    if (tonemapper == TONEMAP_REINHARD)
        color = reinhard(color);
    else if (tonemapper == TONEMAP_ACES)
        color = acesFitted(color);
    color = clamp(color, 0.0, 1.0);
    if (srgbEncode)
        color = linearToSrgb(color);
    fragColor = vec4(color, 1.0);
}
