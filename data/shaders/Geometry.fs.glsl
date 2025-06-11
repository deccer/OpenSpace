#version 460 core

//#extension GL_ARB_bindless_texture : enable
#extension GL_NV_gpu_shader5 : enable
#extension GL_ARB_gpu_shader_int64 : enable

layout(location = 0) in vec3 v_normal;
layout(location = 1) in vec4 v_tangent_with_sign;
layout(location = 4) in vec2 v_uv;
layout(location = 5) in vec4 v_current_world_position;
layout(location = 6) in vec4 v_previous_world_position;
layout(location = 7) flat in uint v_material_id;

layout(location = 0) out vec4 o_color;
layout(location = 1) out vec4 o_normal_ao;
layout(location = 2) out vec4 o_velocity_roughness_metalness;

//layout(binding = 0) uniform sampler2D u_sampler_shadow;
//layout(binding = 1) uniform sampler2D u_sampler_grid;

layout(binding = 8) uniform sampler2D u_texture_base_color;
layout(binding = 9) uniform sampler2D u_texture_normal;
layout(binding = 10) uniform sampler2D u_texture_arm;

struct TGpuMaterial
{
    vec4 base_color;

    uint64_t base_texture_handle;
    uint64_t normal_texture_handle;
    uint64_t occlusion_texture_handle;
    uint64_t metallic_roughness_texture_handle;

    uint64_t emissive_texture_handle;
    uint64_t _padding1;
};

layout (binding = 4, std430) readonly buffer TGpuMaterialBuffer
{
    TGpuMaterial GpuMaterials[];
};

void main()
{
    TGpuMaterial material = GpuMaterials[v_material_id];

    vec3 N = normalize(v_normal);
    vec3 T = normalize(v_tangent_with_sign.xyz);
    vec3 B = cross(N, T) * v_tangent_with_sign.w;
    mat3 TBN = mat3(T, B, N);
    vec3 normal = normalize(TBN * (normalize(texture(u_texture_normal, v_uv).rgb) * 2.0 - vec3(1.0)));

    vec3 ao_roughness_metalness = texture(u_texture_arm, v_uv).rgb;

    o_color = vec4(texture(u_texture_base_color, v_uv).rgb, 1.0);
    o_normal_ao = vec4(normal, ao_roughness_metalness.r);

    vec2 currentPosition = v_current_world_position.xy / v_current_world_position.w;
    vec2 previousPosition = v_previous_world_position.xy / v_previous_world_position.w;
    vec2 velocity = (currentPosition - previousPosition) * 0.5 + 0.5; // map [-1,1] ndc to [0,1]

    o_velocity_roughness_metalness = vec4(velocity, ao_roughness_metalness.gb);
}
