#version 460 core

#include "VertexTypes.include.glsl"

layout (location = 0) out gl_PerVertex
{
    vec4 gl_Position;
};
layout (location = 0) out vec3 v_normal;
layout (location = 1) out vec2 v_uv;
layout (location = 2) out mat3 v_tbn;
layout (location = 6) flat out uint v_material_id;

layout (location = 0, std140) uniform TGpuGlobalUniformBuffer
{
    mat4 ProjectionMatrix;
    mat4 ViewMatrix;
    vec4 CameraPosition;
} u_camera_information;

layout(binding = 1, std430) restrict readonly buffer TGpuVertexPositionBuffer
{
    TVertexPosition VertexPositions[];
};

layout(binding = 2, std430) restrict readonly buffer TGpuVertexNormalUvTangentBuffer
{
    TVertexNormalTangentUvSign VertexNormalUvTangents[];
};

//#include "ObjectBuffer.include.glsl"

layout(location = 0) uniform mat4 u_object_world_matrix;
layout(location = 4) uniform ivec4 u_object_parameters;

#include "BasicFunctions.include.glsl"

void main()
{
    TVertexPosition vertex_position = VertexPositions[gl_VertexID];
    TVertexNormalTangentUvSign vertex_normal_uv_tangent = VertexNormalUvTangents[gl_VertexID];
    //SObject object = Objects[gl_InstanceID];

    v_normal = DecodeNormal(unpackSnorm2x16(vertex_normal_uv_tangent.Normal));
    vec3 tangent = DecodeNormal(unpackSnorm2x16(vertex_normal_uv_tangent.Tangent));
    vec4 uv_and_tangent_sign = PackedToVec4(vertex_normal_uv_tangent.UvAndTangentSign);
    v_uv = uv_and_tangent_sign.xy;
    vec4 t = vec4(tangent, uv_and_tangent_sign.z);
    //v_material_id = object.InstanceParameter.x;
    v_material_id = u_object_parameters.x;

    vec3 normal = normalize(vec3(u_object_world_matrix * vec4(v_normal, 0.0)));
    tangent = normalize(vec3(u_object_world_matrix * vec4(t.xyz, 0.0)));
    vec3 bitangent = normalize(cross(normal, tangent)) * t.w;

    v_tbn = mat3(tangent, bitangent, normal);

    //v_normal = normalize(inverse(transpose(mat3(u_object_world_matrix))) * v_normal);

    gl_Position = u_camera_information.ProjectionMatrix *
                  u_camera_information.ViewMatrix *
//                  object.WorldMatrix * 
                  u_object_world_matrix *
                  vec4(PackedToVec3(vertex_position.Position), 1.0);
}