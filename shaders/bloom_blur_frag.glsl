#version 410

uniform sampler2D image;
uniform bool horizontal;

in vec2 texCoord;

layout(location = 0) out vec4 fragColor;

// Gaussian weights for the center and 4 pixels on each side
const float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main()
{
    vec2 texel = 1.0 / vec2(textureSize(image, 0));
    vec2 direction = horizontal ? vec2(texel.x, 0.0) : vec2(0.0, texel.y);

    // weighted sum along one direction
    vec3 result = texture(image, texCoord).rgb * weights[0];
    for (int i = 1; i < 5; ++i) {
        result += texture(image, texCoord + direction * float(i)).rgb * weights[i];
        result += texture(image, texCoord - direction * float(i)).rgb * weights[i];
    }

    fragColor = vec4(result, 1.0);
}
