#version 460 core

layout(location = 1) in vec3 v_sky_ray;

layout(location = 0) out vec4 o_color;

layout(binding = 0) uniform sampler2D s_texture_depth;
layout(binding = 1) uniform sampler2D s_texture_gbuffer_albedo;
layout(binding = 2) uniform sampler2D s_texture_gbuffer_normal;
layout(binding = 2) uniform sampler2D s_texture_gbuffer_velocity_metallic_roughness;

layout(binding = 8) uniform samplerCube s_environment;
layout(binding = 9) uniform samplerCube s_irradiance_environment;
layout(binding = 10) uniform samplerCube s_prefiltered_radiance_environment;
layout(binding = 11) uniform sampler2D s_brdf_lut;

layout(location = 0) uniform mat4 u_camera_inverse_view_projection;
layout(location = 4) uniform vec4 u_camera_position;
layout(location = 5) uniform vec3 u_sun_position;
layout(location = 6) uniform vec2 u_screen_size;

struct TGpuGlobalLight {
    vec4 Direction;
    vec4 ColorAndIntensity;
    ivec4 Properties;
};

layout(binding = 1, std140) uniform GlobalLights {
    TGpuGlobalLight Lights[8];
} u_global_lights;

#include "Include.Pi.glsl"

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 F_Schlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 F_SchlickR(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

// Normal Distribution function --------------------------------------
float D_GGX(float dotNH, float roughness) {
    float alpha = roughness * roughness;
    float alphaSquared = alpha * alpha;
    float denom = dotNH * dotNH * (alphaSquared - 1.0) + 1.0;
    return (alphaSquared) / (PI * denom * denom);
}

// Geometric Shadowing function --------------------------------------
float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float GL = dotNL / (dotNL * (1.0 - k) + k);
    float GV = dotNV / (dotNV * (1.0 - k) + k);
    return GL * GV;
}

vec3 GetPrefilteredReflection(vec3 R, float roughness) {
    const float MAX_REFLECTION_LOD = 9.0; // 512x512 BRDF LUT
    float lod = roughness * MAX_REFLECTION_LOD;
    float lodf = floor(lod);
    float lodc = ceil(lod);
    vec3 a = textureLod(s_prefiltered_radiance_environment, R, lodf).rgb;
    vec3 b = textureLod(s_prefiltered_radiance_environment, R, lodc).rgb;
    return mix(a, b, lod - lodf);
}

vec3 SpecularContribution(vec3 L, vec3 V, vec3 N, vec3 F0, float metallic, float roughness, vec3 albedo) {

    // Precalculate vectors and dot products
    vec3 H = normalize (V + L);
    float dotNH = clamp(dot(N, H), 0.0, 1.0);
    float dotNV = clamp(dot(N, V), 0.0, 1.0);
    float dotNL = clamp(dot(N, L), 0.0, 1.0);

    vec3 color = vec3(0.0);

    if (dotNL > 0.0) {
        // D = Normal distribution (Distribution of the microfacets)
        float D = D_GGX(dotNH, roughness);
        // G = Geometric shadowing term (Microfacets shadowing)
        float G = G_SchlicksmithGGX(dotNL, dotNV, roughness);
        // F = Fresnel factor (Reflectance depending on angle of incidence)
        vec3 F = FresnelSchlick(dotNV, F0);
        vec3 specular = D * F * G / (4.0 * dotNL * dotNV + 0.001);
        vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
        color += (kD * albedo / PI + specular) * dotNL;
    }

    return color;
}

vec3 ReconstructFragmentWorldPositionFromDepth(float depth, vec2 screenSize, mat4 invViewProj) {

    float z = depth * 2.0 - 1.0; // [0, 1] -> [-1, 1]
    vec2 position_cs = gl_FragCoord.xy / (screenSize - 1); // [0.5, screenSize] -> [0, 1]
    vec4 position_ndc = vec4(position_cs * 2.0 - 1.0, z, 1.0); // [0, 1] -> [-1, 1]

    // undo view + projection
    vec4 position_ws = invViewProj * position_ndc;
    position_ws /= position_ws.w;

    return position_ws.xyz;
}

void main()
{
    float depth = texelFetch(s_texture_depth, ivec2(gl_FragCoord.xy), 0).r;
    vec4 albedo = texelFetch(s_texture_gbuffer_albedo, ivec2(gl_FragCoord.xy), 0);
    vec4 normal_ao = texelFetch(s_texture_gbuffer_normal, ivec2(gl_FragCoord.xy), 0);
    vec4 velocity_metallic_roughness = texelFetch(s_texture_gbuffer_velocity_metallic_roughness, ivec2(gl_FragCoord.xy), 0);
    vec3 normal = normal_ao.xyz;

    if (depth >= 1.0) {
        vec3 color = texture(s_environment, v_sky_ray).rgb;

        o_color = vec4(color, 1.0);
        return;
    }

    vec3 fragmentPosition_ws = ReconstructFragmentWorldPositionFromDepth(depth, u_screen_size, u_camera_inverse_view_projection);
    vec3 V = normalize(u_camera_position.xyz - fragmentPosition_ws);
    vec3 N = normalize(normal);
    vec3 R = reflect(-V, N);

    float metallic = velocity_metallic_roughness.b;
    float roughness = velocity_metallic_roughness.a;
    vec3 F0 = mix(vec3(0.04), albedo.rgb, metallic);

    vec3 Lo = vec3(0.0);
    for (int i = 0; i < 8; i++) {
        if (u_global_lights.Lights[i].Properties.x == 1) {
            vec3 L = normalize(u_global_lights.Lights[i].Direction.xyz - fragmentPosition_ws);
            Lo += SpecularContribution(L, V, N, F0, metallic, roughness, u_global_lights.Lights[i].ColorAndIntensity.rgb);
        }
    }

    vec3 irradiance = texture(s_irradiance_environment, N).rgb;
    vec3 reflection = GetPrefilteredReflection(R, roughness);
    vec2 brdf = texture(s_brdf_lut, vec2(max(dot(N, V), 0.0), roughness)).rg;

    // Diffuse based on irradiance
    vec3 diffuse = irradiance * albedo.rgb;
    vec3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

    // Specular reflectance
    vec3 specular = reflection * (F * brdf.x + brdf.y);

    // Ambient part
    vec3 kD = 1.0 - F;
    kD *= 1.0 - metallic;
    vec3 ambient = (kD * diffuse + specular) * normal_ao.w;

    vec3 color = ambient + Lo;
    if (dot(N, V) < 0) {
        color = vec3(1, 0, 1);
    }

    //o_color = vec4(N, 1.0);
    o_color = vec4(N, 1.0);
    //o_color = 0.0000001 * vec4(color, 1.0) + vec4(ambient, 0.0);
}
