#version 460 core

#include "Include.VertexTypes.glsl"

layout(location = 0) out mat3 v_tbn;
layout(location = 4) out vec3 v_normal;
layout(location = 5) out vec3 v_tangent;
layout(location = 6) out vec2 v_uv;
layout(location = 7) out vec4 v_current_world_position;
layout(location = 8) out vec4 v_previous_world_position;
layout(location = 9) flat out uint v_material_id;

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

layout(binding = 2, std430) restrict readonly buffer TGpuVertexNormalUvTangentBuffer
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

    vec3 decoded_normal = DecodeOctahedral(unpackSnorm2x16(vertex_normal_uv_tangent.Normal));
    vec3 decoded_tangent = DecodeOctahedral(unpackSnorm2x16(vertex_normal_uv_tangent.Tangent));
    vec4 decoded_uv_and_tangent_sign = PackedToVec4(vertex_normal_uv_tangent.UvAndTangentSign);

    // MikkTSpace already generates orthogonalized tangents in object space
    v_normal = normalize((u_object_world_matrix * vec4(decoded_normal, 0.0)).xyz);
    vec3 tangent_ws = normalize((u_object_world_matrix * vec4(decoded_tangent, 0.0)).xyz);

    // re-orthogonalize in case world matrix has non-uniform scale
    vec3 tangent_orthogonal = tangent_ws - dot(tangent_ws, v_normal) * v_normal;
    float tangent_len = length(tangent_orthogonal);
    v_tangent = tangent_len > 0.0001 ? tangent_orthogonal / tangent_len : tangent_ws;

    float tangentSign = decoded_uv_and_tangent_sign.z;
    vec3 bitangent = cross(v_normal, v_tangent) * tangentSign;

    v_tbn = mat3(v_tangent, bitangent, v_normal);
    v_uv = decoded_uv_and_tangent_sign.xy;
    v_material_id = u_object_parameters.x;

    vec4 worldPosition = u_object_world_matrix * vec4(PackedToVec3(vertex_position.Position), 1.0);
    v_current_world_position = u_camera_information.CurrentJitteredViewProjectionMatrix * worldPosition;
    v_previous_world_position = u_camera_information.PreviousJitteredViewProjectionMatrix * worldPosition;

    gl_Position = v_current_world_position;
}
