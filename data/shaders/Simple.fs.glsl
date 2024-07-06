#version 460 core

#extension GL_ARB_bindless_texture : require
#extension GL_NV_gpu_shader5 : require
#extension GL_ARB_gpu_shader_int64 : require

layout(location = 0) in vec3 v_normal;
layout(location = 1) in vec2 v_uv;
layout(location = 2) in vec4 v_tangent;
layout(location = 3) flat in uint v_material_id;

layout(location = 0) out vec4 o_color;
layout(location = 1) out vec4 o_normal;

layout(binding = 0) uniform sampler2D u_shadow_map;
layout(binding = 1) uniform sampler2D u_grid_texture;

layout(location = 8) uniform uint64_t u_base_color_texture;
layout(location = 9) uniform uint64_t u_normal_texture;

struct SGpuMaterial
{
    vec4 base_color;

    uint64_t base_texture_handle;
    uint64_t normal_texture_handle;
    uint64_t occlusion_texture_handle;
    uint64_t metallic_roughness_texture_handle;

    uint64_t emissive_texture_handle;
    uint64_t _padding1;
};

layout (binding = 4, std430) readonly buffer GpuMaterialBuffer
{
    SGpuMaterial GpuMaterials[];
};

layout (binding = 2, std140) uniform GlobalLights {
    mat4 ProjectionMatrix;
    mat4 ViewMatrix;
    vec4 Direction;
    vec4 Strength;
} u_global_lights[8];

float GetShadow() {
    return 1.0f;
}

void main()
{
    SGpuMaterial material = GpuMaterials[v_material_id];
    
    const float sun_n_dot_l = clamp(dot(v_normal, -u_global_lights[0].Direction.xyz), 0.0, 1.0);

    //texture(sampler2D(material.base_texture_handle), v_uv)

    vec3 normal = u_normal_texture == 0
        ? v_normal
        : texture(sampler2D(u_normal_texture), v_uv).rgb;

    //o_color = vec4(v_uv, 0.0f, 1.0f) * vec4(u_global_lights[0].Strength.rgb * sun_n_dot_l, 1.0);
    //o_color = vec4(v_normal * 0.5 + 0.5, 1.0f);// * vec4(u_global_lights[0].Strength.rgb * sun_n_dot_l, 1.0);
    o_color = texture(sampler2D(u_base_color_texture), v_uv);
    o_normal = vec4(normal * 0.5 + 0.5, 1.0);
}