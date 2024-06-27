#version 450

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec3 fragNormal;
layout (location = 2) in vec2 fragUV;

layout (set = 2, binding = 0) uniform sampler2D colorSampler;

layout (location = 0) out vec4 outColor;

void main()
{
    outColor = texture(colorSampler, fragUV) * vec4(fragColor, 1.0);
}
