#version 460 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec4 in_color;

layout(location = 0) out vec4 v_color;

layout (binding = 0, std140) uniform TGpuGlobalUniformBuffer {
    mat4 ProjectionMatrix;
    mat4 ViewMatrix;
    vec4 CameraPosition; // xyz = position, w = fieldOfView
    vec4 CameraDirection; // xyz = direction, w = aspectRatio
} u_global_uniforms;

void main()
{
    v_color = in_color;
    gl_Position = u_global_uniforms.ProjectionMatrix *
                  u_global_uniforms.ViewMatrix *
                  vec4(in_position, 1.0);
}
