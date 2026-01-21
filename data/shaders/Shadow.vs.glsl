#version 460 core

#include "Include.VertexTypes.glsl"

layout (location = 0) uniform int u_global_light_index;
layout (location = 1) uniform mat4 u_world_matrix;

layout (location = 0) out gl_PerVertex
{
    vec4 gl_Position;
};

layout(binding = 1, std430) restrict readonly buffer TVertexPositionBuffer
{
    TVertexPosition VertexPositions[];
};

struct TGpuGlobalLight {
    mat4 ShadowViewProjectionMatrix;
    vec4 Direction;
    vec4 ColorAndIntensity;
    ivec4 LightProperties;
};

layout (binding = 2, std140) uniform TGlobalLights {
    TGpuGlobalLight Lights[8];
} u_global_lights;

void main()
{
    TVertexPosition vertex_position = VertexPositions[gl_VertexID];
    gl_Position = u_global_lights.Lights[u_global_light_index].ShadowViewProjectionMatrix *
                  u_world_matrix *
                  vec4(PackedToVec3(vertex_position.Position), 1.0);
}
