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

layout (set = 1, binding = 0) uniform ModelMatrix {
    mat4 UniqueModel;
} model;

layout (location = 0) out vec3 fragColor;
layout (location = 1) out vec3 fragNormal;
layout (location = 2) out vec2 fragUV;

void main()
{
    gl_Position = cam.Projection * cam.View * model.UniqueModel * vec4(position, 1.0);
    fragColor = color;
    fragNormal = normal;
    fragUV = uv;
}
