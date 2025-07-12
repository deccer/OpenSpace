#version 460 core

#include "Include.VertexTypes.glsl"

layout (location = 0) uniform mat4 u_object_world_matrix;
layout (location = 4) uniform mat4 u_light_matrix;

layout(binding = 1, std430) restrict readonly buffer TVertexPositionBuffer {
    TVertexPosition VertexPositions[];
};

void main()
{
    TVertexPosition vertex_position = VertexPositions[gl_VertexID];
    gl_Position = u_light_matrix *
                  u_object_world_matrix *
                  vec4(PackedToVec3(vertex_position.Position), 1.0);
}
