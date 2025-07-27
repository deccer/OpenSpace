#version 460 core

#include "Include.VertexTypes.glsl"

layout (location = 0) uniform mat4 u_light_view_projection_matrix;
layout (location = 4) uniform mat4 u_object_world_matrix;

layout(binding = 1, std430) restrict readonly buffer TVertexPositionBuffer {
    TVertexPosition VertexPositions[];
};

void main()
{
    TVertexPosition vertex_position = VertexPositions[gl_VertexID];
    //vec4 in_position = vec4(PackedToVec4(vertex_position.Position).xyz, 1.0);
    vec4 in_position = vec4(vertex_position.Position.xyz, 1.0);

    gl_Position = u_light_view_projection_matrix *
                  u_object_world_matrix *
                  in_position;
}
