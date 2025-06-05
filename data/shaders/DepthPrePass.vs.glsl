#version 460 core

#include "Include.VertexTypes.glsl"

layout (location = 0) out gl_PerVertex
{
    vec4 gl_Position;
};
layout (location = 0) out vec3 v_normal;
layout (location = 1) out vec2 v_uv;
layout (location = 2) out mat3 v_tbn;

layout (binding = 0, std140) uniform TGpuGlobalUniformBuffer
{
    mat4 ProjectionMatrix;
    mat4 ViewMatrix;
    mat4 CurrentJitteredViewProjectionMatrix;
    mat4 PreviousJitteredViewProjectionMatrix;
    vec4 CameraPosition; // xyz = position, w = fieldOfView
    vec4 CameraDirection; // xyz = direction, w = aspectRatio
} u_camera_information;

layout(binding = 1, std430) restrict readonly buffer TGpuVertexPositionBuffer
{
    TVertexPosition VertexPositions[];
};

layout(location = 0) uniform mat4 u_object_world_matrix;

void main()
{
    TVertexPosition vertex_position = VertexPositions[gl_VertexID];
/*
    gl_Position = u_camera_information.ProjectionMatrix *
                  u_camera_information.ViewMatrix *
                  u_object_world_matrix *
                  vec4(PackedToVec3(vertex_position.Position), 1.0);
*/
    gl_Position = u_camera_information.CurrentJitteredViewProjectionMatrix *
                  u_object_world_matrix *
                  vec4(PackedToVec3(vertex_position.Position), 1.0);
}
