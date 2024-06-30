#version 460 core

#include "VertexTypes.include.glsl"

layout (location = 0) out gl_PerVertex
{
    vec4 gl_Position;
};
layout (location = 0) out vec3 v_normal;
layout (location = 1) out vec2 v_uv;
layout (location = 2) out vec4 v_tangent;
layout (location = 3) flat out uint v_material_id;

layout (location = 0, std140) uniform SGpuGlobalUniformBuffer
{
    mat4 ProjectionMatrix;
    mat4 ViewMatrix;
    vec4 CameraPosition;
} u_camera_information;

layout(binding = 1, std430) restrict readonly buffer SGpuVertexPositionBuffer
{
    SVertexPosition VertexPositions[];
};

layout(binding = 2, std430) restrict readonly buffer SGpuVertexNormalUvTangentBuffer
{
    SVertexNormalUvTangent VertexNormalUvTangents[];
};

//#include "ObjectBuffer.include.glsl"

layout(location = 0) uniform mat4 u_object_world_matrix;
layout(location = 4) uniform ivec4 u_object_parameters;

#include "BasicFunctions.include.glsl"

void main()
{
    SVertexPosition vertex_position = VertexPositions[gl_VertexID];
    SVertexNormalUvTangent vertex_normal_uv_tangent = VertexNormalUvTangents[gl_VertexID];
    //SObject object = Objects[gl_InstanceID];

    v_normal = DecodeNormal(unpackSnorm2x16(vertex_normal_uv_tangent.Normal));
    vec3 tangent = DecodeNormal(unpackSnorm2x16(vertex_normal_uv_tangent.Tangent));
    vec3 uv_and_tangent_sign = PackedToVec3(vertex_normal_uv_tangent.UvAndTangentSign);
    v_uv = uv_and_tangent_sign.xy;
    v_tangent = vec4(tangent, uv_and_tangent_sign.z);
    //v_material_id = object.InstanceParameter.x;
    v_material_id = u_object_parameters.x;

    v_normal = normalize(inverse(transpose(mat3(u_object_world_matrix))) * v_normal);

    gl_Position = u_camera_information.ProjectionMatrix *
                  u_camera_information.ViewMatrix *
//                  object.WorldMatrix * 
                  u_object_world_matrix *
                  vec4(PackedToVec3(vertex_position.Position), 1.0);
}