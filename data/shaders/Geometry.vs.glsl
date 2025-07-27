#version 460 core

#include "Include.VertexTypes.glsl"

layout(location = 0) out mat3 v_tbn;
layout(location = 4) out vec3 v_normal;
layout(location = 5) out vec3 v_tangent;
layout(location = 6) out vec2 v_uv;
layout(location = 7) out vec4 v_fragment_position_cs;
layout(location = 8) out vec4 v_fragment_position_vs;
layout(location = 9) flat out uint v_material_id;

layout (binding = 0, std140) uniform TGpuGlobalUniformBuffer {
    mat4 ProjectionMatrix;
    mat4 ViewMatrix;
    vec4 CameraPosition; // xyz = position, w = fieldOfView
    vec4 CameraDirection; // xyz = direction, w = aspectRatio
} u_global_uniforms;

layout(binding = 1, std430) restrict readonly buffer TGpuVertexPositionBuffer
{
    TVertexPosition VertexPositions[];
};

layout(binding = 2, std430) restrict readonly buffer TGpuVertexNormalTangentUvBuffer
{
    TPackedVertexNormalTangentUvSign VertexNormalUvTangents[];
};

layout(location = 0) uniform mat4 u_object_world_matrix;
layout(location = 4) uniform ivec4 u_object_parameters;

#include "Include.BasicFunctions.glsl"

void main()
{
    TVertexPosition vertex_position = VertexPositions[gl_VertexID];
    TPackedVertexNormalTangentUvSign vertex_normal_uv_tangent = VertexNormalUvTangents[gl_VertexID];

    //vec4 in_position = vec4(PackedToVec4(vertex_position.Position).xyz, 1.0);
    vec4 in_position = vec4(vertex_position.Position.xyz, 1.0);
    vec3 decoded_normal = DecodeOctahedral(unpackSnorm2x16(vertex_normal_uv_tangent.Normal));
    vec3 decoded_tangent = DecodeOctahedral(unpackSnorm2x16(vertex_normal_uv_tangent.Tangent));
    vec4 decoded_uv_and_tangent_sign = PackedToVec4(vertex_normal_uv_tangent.UvAndTangentSign);

    v_normal = normalize((u_object_world_matrix * vec4(decoded_normal, 0.0)).xyz);
    v_tangent = normalize((u_object_world_matrix * vec4(decoded_tangent, 0.0)).xyz);
    v_tangent = normalize(v_tangent - dot(v_tangent, v_normal) * v_normal);

    vec3 bitangent = cross(v_tangent, v_normal);
    v_tbn = mat3(v_tangent, bitangent, v_normal);
    v_uv = decoded_uv_and_tangent_sign.xy;
    v_material_id = u_object_parameters.x;

    vec4 fragment_position_ws = u_object_world_matrix * in_position;
    v_fragment_position_vs = u_global_uniforms.ViewMatrix * fragment_position_ws;
    v_fragment_position_cs = u_global_uniforms.ProjectionMatrix * v_fragment_position_vs;

    gl_Position = v_fragment_position_cs;
}
