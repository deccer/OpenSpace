#version 460 core

//#extension GL_ARB_bindless_texture : enable
#extension GL_NV_gpu_shader5 : enable
#extension GL_ARB_gpu_shader_int64 : enable

layout(location = 0) in mat3 v_tbn;
layout(location = 4) in vec3 v_normal;
layout(location = 5) in vec3 v_tangent;
layout(location = 6) in vec2 v_uv;
layout(location = 7) in vec4 v_current_world_position;
layout(location = 8) in vec4 v_previous_world_position;
layout(location = 9) in vec3 v_fragment_position_vs;
layout(location = 10) flat in uint v_material_id;

layout(location = 0) out vec4 o_color;
layout(location = 1) out vec4 o_normal_ao;
layout(location = 2) out vec4 o_velocity_roughness_metalness;
layout(location = 3) out vec4 o_emissive_viewz;

//layout(binding = 0) uniform sampler2D u_sampler_shadow;
//layout(binding = 1) uniform sampler2D u_sampler_grid;

layout(binding = 8) uniform sampler2D u_texture_base_color;
layout(binding = 9) uniform sampler2D u_texture_normal;
layout(binding = 10) uniform sampler2D u_texture_arm;
layout(binding = 11) uniform sampler2D u_texture_emissive;

layout(location = 5) uniform vec4 u_factors;
layout(location = 6) uniform vec4 u_material_emissive;
layout(location = 7) uniform uint u_texture_flags;

struct TGpuMaterial
{
    vec4 base_color;
    vec4 factors; // normal strength, metalness, roughness, emission strength
    vec4 emissive_color;

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

vec3 normalMap(mat3 tbn, vec3 N) {
    N.y = 1.0 - N.y;
    return normalize(tbn * (N * 2.0 - 1.0));
}

void main()
{
    TGpuMaterial material = GpuMaterials[v_material_id];

    bool hasBaseColorTexture = (u_texture_flags & 1u) != 0u;
    bool hasNormalTexture = (u_texture_flags & 2u) != 0u;
    bool hasArmTexture = (u_texture_flags & 4u) != 0u;
    bool hasEmissiveTexture = (u_texture_flags & 8u) != 0u;

    vec3 ao_roughness_metalness = vec3(1.0, u_factors.g, u_factors.b);
    if (hasArmTexture) {
        ao_roughness_metalness = texture(u_texture_arm, v_uv).rgb;
    }

    o_color = vec4(0.5, 0.5, 0.5, 1.0);
    if (hasBaseColorTexture) {
        o_color = vec4(texture(u_texture_base_color, v_uv).rgb, 1.0);
    }

    o_normal_ao = vec4(v_normal, ao_roughness_metalness.r);
    if (hasNormalTexture) {
        vec3 sampledNormal = texture(u_texture_normal, v_uv).xyz;
        sampledNormal.y = 1.0 - sampledNormal.y;
        vec3 normal = normalize(v_tbn * (sampledNormal * 2.0 - 1.0));
        o_normal_ao = vec4(normal, ao_roughness_metalness.r);
    }

    vec2 currentPosition = v_current_world_position.xy / v_current_world_position.w;
    vec2 previousPosition = v_previous_world_position.xy / v_previous_world_position.w;
    vec2 velocity = (currentPosition - previousPosition) * 0.5 + 0.5; // map [-1,1] ndc to [0,1]

    o_velocity_roughness_metalness = vec4(velocity, ao_roughness_metalness.gb);

    o_emissive_viewz = vec4(u_material_emissive.rgb, v_fragment_position_vs.z);
    if (hasEmissiveTexture) {
        o_emissive_viewz = vec4(texture(u_texture_emissive, v_uv).rgb, 1.0);
    }
}
