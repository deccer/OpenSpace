#version 460 core

#extension GL_ARB_bindless_texture : require
#extension GL_NV_gpu_shader5 : require
#extension GL_ARB_gpu_shader_int64 : require

layout (location = 0) in vec3 v_normal;
layout (location = 1) in vec2 v_uv;
layout (location = 2) flat in uint v_material_id;

layout (location = 0) out vec4 o_color;
layout (location = 1) out vec4 o_normal;

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

layout (binding = 3, std430) readonly buffer GpuMaterialBuffer
{
    SGpuMaterial GpuMaterials[];
};

vec3 HsvToRgb(in vec3 hsv)
{
    vec3 rgb = clamp(abs(mod(hsv.x * 6.0 + vec3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0);
    return hsv.z * mix(vec3(1.0), rgb, hsv.y);
}

const vec3 k_colors[8] = 
{
    vec3(1.0, 0.0, 0.0),
    vec3(1.0, 1.0, 0.0),
    vec3(1.0, 1.0, 1.0),
    vec3(0.0, 1.0, 1.0),
    vec3(0.0, 0.0, 1.0),
    vec3(1.0, 0.0, 1.0),
    vec3(1.0, 1.0, 0.9),
    vec3(0.5, 0.5, 0.5)
};

void main()
{
    const float GOLDEN_CONJ = 0.6180339887498948482045868343656;

    SGpuMaterial material = GpuMaterials[v_material_id];
    o_color = vec4(k_colors[v_material_id], 1.0);

    int colorIndex = int(material.base_texture_handle - 0x1000009FFul);//bitfieldExtract(int(material.base_texture >> 32), 0, 8);
    o_normal = vec4(k_colors[colorIndex], 1.0);
}