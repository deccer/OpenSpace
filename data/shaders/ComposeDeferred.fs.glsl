#version 460 core

#extension GL_NV_uniform_buffer_std430_layout : enable

layout(location = 1) in vec3 v_sky_ray;

layout(location = 0) out vec4 o_color;

layout(binding = 0) uniform sampler2D s_texture_depth;
layout(binding = 1) uniform sampler2D s_texture_gbuffer_albedo;
layout(binding = 2) uniform sampler2D s_texture_gbuffer_normal_ao;
layout(binding = 3) uniform sampler2D s_texture_gbuffer_velocity_roughness_metallic;
layout(binding = 4) uniform sampler2D s_texture_gbuffer_emissive;
layout(binding = 5) uniform sampler2D s_texture_lbuffer;

layout(binding = 8) uniform samplerCube s_environment;
layout(binding = 9) uniform samplerCube s_irradiance_environment;
layout(binding = 10) uniform samplerCube s_prefiltered_radiance_environment;
layout(binding = 11) uniform sampler2D s_brdf_lut;
//layout(binding = 12) uniform sampler2DArrayShadow s_shadowMap;

layout(location = 0) uniform mat4 u_camera_inverse_view_projection;
layout(location = 4) uniform vec4 u_camera_position;
layout(location = 5) uniform vec2 u_screen_size;

/*
const int MAX_SHADOW_CASCADES = 4;

struct TGpuGlobalLight {
    mat4 LightProjectionView[MAX_SHADOW_CASCADES];
    vec4 Direction;
    vec4 ColorAndIntensity;
    ivec4 Properties;
    float ShadowCascadeDepths[8];
};

layout(binding = 1, std430) uniform GlobalLights {
    TGpuGlobalLight Lights[8];
} u_global_lights;
*/

#include "Include.Pi.glsl"
#include "Include.FresnelSchlick.glsl"
#include "Include.CalculatePrefilteredReflection.glsl"
#include "Include.Fresnel.glsl"
#include "Include.SpecularContribution.glsl"
#include "Include.ReconstructFragmentWorldPositionFromDepth.glsl"

vec3 debugColorForCascade(int cascadeIndex) {
    if (cascadeIndex == 0) return vec3(1, 0, 0);     // red
    if (cascadeIndex == 1) return vec3(0, 1, 0);     // green
    if (cascadeIndex == 2) return vec3(0, 0, 1);     // blue
    if (cascadeIndex == 3) return vec3(1, 1, 0);     // yellow
    return vec3(1); // fallback
}

/*
float GetShadow(TGpuGlobalLight light, vec3 fragmentPositionWs, float fragmentPositionZVs, vec3 normalWs) {

    int shadowCascade = MAX_SHADOW_CASCADES - 1;
    for (int i = 0; i < MAX_SHADOW_CASCADES; i++) {
        if (fragmentPositionZVs < light.ShadowCascadeDepths[i]) {
            shadowCascade = i;
            break;
        }
    }

    vec4 fragPosLightSpace = light.LightProjectionView[shadowCascade] * vec4(fragmentPositionWs, 1.0);
    vec3 fragPosShadowSpace = fragPosLightSpace.xyz * 0.5 + 0.5;

    // Early exit if the fragment is outside the light's view
    if (fragPosShadowSpace.z > 1.0) {
        return 1.0;
    }

    const vec2 poissonDisk[16] = vec2[](
        vec2(0.97484398, 0.75648379), vec2(0.94558609, -0.76890725),
        vec2(-0.81544232, -0.87912464), vec2(-0.81409955, 0.91437590),
        vec2(-0.94201624, -0.39906216), vec2(-0.094184101, -0.92938870),
        vec2(0.34495938, 0.29387760), vec2(-0.91588581, 0.45771432),
        vec2(-0.38277543, 0.27676845), vec2(0.44323325, -0.97511554),
        vec2(0.53742981, -0.47373420), vec2(-0.26496911, -0.41893023),
        vec2(0.79197514, 0.19090188), vec2(-0.24188840, 0.99706507),
        vec2(0.19984126, 0.78641367), vec2(0.14383161, -0.14100790)
    );

    float shadow = 0.0;
    float bias = max(0.005 * (1.0 - dot(normalWs, normalize(-light.Direction.xyz))), 0.0005);

    for (int i = 0; i < 16; i++) {
        shadow += texture(s_shadowMap, vec4(fragPosShadowSpace.xy + poissonDisk[i] / 1000.0, shadowCascade, fragPosShadowSpace.z - bias));
    }

    return shadow / 16.0;
}
*/

void main() {
    float depth = texelFetch(s_texture_depth, ivec2(gl_FragCoord.xy), 0).r;

    if (depth >= 1.0) {
        vec3 color = texture(s_environment, v_sky_ray).rgb;

        o_color = vec4(color, 1.0);
        return;
    }

    vec4 albedo = texelFetch(s_texture_gbuffer_albedo, ivec2(gl_FragCoord.xy), 0);
    vec4 normal_ao = texelFetch(s_texture_gbuffer_normal_ao, ivec2(gl_FragCoord.xy), 0);
    vec4 velocity_roughness_metallic = texelFetch(s_texture_gbuffer_velocity_roughness_metallic, ivec2(gl_FragCoord.xy), 0);
    vec4 emissive_viewz = texelFetch(s_texture_gbuffer_emissive, ivec2(gl_FragCoord.xy), 0);
    vec3 emissive = emissive_viewz.rgb;
    float viewz = emissive_viewz.a;
    vec3 normal = normal_ao.xyz;

    vec3 fragmentPosition_ws = ReconstructFragmentWorldPositionFromDepth(depth, u_screen_size, u_camera_inverse_view_projection);
    vec3 V = normalize(u_camera_position.xyz - fragmentPosition_ws);
    vec3 N = normalize(normal);
    vec3 R = reflect(-V, N);

    float roughness = velocity_roughness_metallic.b;
    float metallic = velocity_roughness_metallic.a;
    vec3 F0 = mix(vec3(0.04), albedo.rgb, metallic);

    //vec3 L = vec3(0.0);
    vec3 Lo = vec3(0.0);

    Lo = texelFetch(s_texture_lbuffer, ivec2(gl_FragCoord.xy), 0).rgb;

    /*
    for (int i = 0; i < 8; i++) {
        TGpuGlobalLight global_light = u_global_lights.Lights[i];
        if (global_light.Properties.x == 1) {

            float shadow = 1.0f;


            if (global_light.Properties.y == 1) {
                shadow = GetShadow(global_light, fragmentPosition_ws, -viewz, normal);
            }


            L = normalize(-global_light.Direction.xyz);
            Lo += SpecularContribution(L, V, N, F0, metallic, roughness, albedo.rgb) * global_light.ColorAndIntensity.rgb * global_light.ColorAndIntensity.a * shadow;
        }
    }
    */

    vec3 irradiance = texture(s_irradiance_environment, N).rgb;
    vec3 reflection = CalculatePrefilteredReflection(R, roughness);
    vec2 brdf = texture(s_brdf_lut, vec2(max(dot(N, V), 0.0), roughness)).rg;

    // diffuse based on irradiance
    vec3 diffuse = irradiance * albedo.rgb;
    vec3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

    // specular reflectance
    vec3 specular = reflection * (F * brdf.x + brdf.y);

    // ambient part
    vec3 kD = 1.0 - F;
    kD *= 1.0 - metallic;
    vec3 ambient = (kD * diffuse + specular) * normal_ao.w;

    vec3 color = ambient + Lo + emissive;
    //if (dot(N, V) < 0) {
    //    color = vec3(1, 0, 1);
    //}

    //o_color = vec4(N, 1.0);
    o_color = vec4(color, 1.0);
    //o_color = 0.0000001 * vec4(color, 1.0) + vec4(normal, 0.0);
    //o_color = 0.0000001 * vec4(color, 1.0) + vec4(debugColorForCascade(shadowCascade), 0.0);

    //o_color = 0.0000001 * vec4(color, 1.0) + vec4(color * (0.5 + pow(max(dot(N, V), 0.0), 5.0) * 2.5), 0.0);
}
