#version 460 core

#include "Include.VertexTypes.glsl"

layout (location = 0) uniform int u_global_light_index;

layout (location = 0) out gl_PerVertex
{
    vec4 gl_Position;
};

layout(binding = 1, std430) restrict readonly buffer TVertexPositionBuffer
{
    TVertexPosition VertexPositions[];
};

struct TGpuGlobalLight {
    mat4 ProjectionMatrix;
    mat4 ViewMatrix;
    vec4 Direction;
    vec4 Strength;
};

layout (binding = 2, std140) uniform TGlobalLights {
    mat4 ProjectionMatrix;
    mat4 ViewMatrix;
    vec4 Direction;
    vec4 Strength;
} u_global_lights[8];

#include "Include.ObjectBuffer.glsl"

void main()
{
    TVertexPosition vertex_position = VertexPositions[gl_VertexID];
    //SGpuGlobalLight global_light = u_global_lights[0];
    gl_Position = (u_global_lights[0].ProjectionMatrix *
                  (u_global_lights[0].ViewMatrix *
                  (Objects[gl_DrawID].WorldMatrix * vec4(PackedToVec3(vertex_position.Position), 1.0))));
}
