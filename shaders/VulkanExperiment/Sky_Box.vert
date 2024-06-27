#version 450

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 uv;

layout (set = 0, binding = 0) uniform CameraUniform {
    mat4 View;
    mat4 InverseView;
    mat4 Projection;
    mat4 InverseProjection;
} cam;

layout (location = 0) out vec3 fragUVW;

void main()
{
    fragUVW = position;
    fragUVW.y *= -1.0;
    gl_Position = (cam.Projection * cam.View * vec4(position.xyz, 1.0)).xyww;
}
