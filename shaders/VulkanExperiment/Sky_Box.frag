#version 450

layout (set = 1, binding = 0) uniform samplerCube cubeMap;

layout (location = 0) in vec3 fragUVW;

layout (location = 0) out vec4 outColor;

void main()
{
    outColor = texture(cubeMap, fragUVW);
}
