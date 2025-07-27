#version 460 core

#extension GL_NV_uniform_buffer_std430_layout : enable

layout(location = 0) out vec4 o_accumulated_light;

layout(binding = 0) uniform sampler2D s_texture_depth;
layout(binding = 1) uniform sampler2D s_texture_gbuffer_albedo;
layout(binding = 2) uniform sampler2D s_texture_gbuffer_normal_ao;
layout(binding = 3) uniform sampler2D s_texture_gbuffer_velocity_roughness_metallic;
layout(binding = 4) uniform sampler2D s_texture_gbuffer_emissive_viewz;

layout(binding = 12) uniform sampler2DArrayShadow s_shadowMap;

layout(location = 0) uniform mat4 u_camera_inverse_view_projection;
layout(location = 4) uniform vec4 u_camera_position;
layout(location = 5) uniform vec2 u_screen_size;
layout(location = 6) uniform int u_global_light_index;

const int MAX_GLOBAL_LIGHTS = 8;
const int MAX_SHADOW_CASCADES = 4;

struct TGpuGlobalLight {
    mat4 LightProjectionView[MAX_SHADOW_CASCADES];
    float ShadowCascadeDepths[8]; // contains 5 values, 8 for alignment

    vec4 Direction;
    vec4 ColorAndIntensity;

    ivec4 Properties; // .x = isEnabled, .y = hasShadows
    ivec4 _padding1;
};

layout(binding = 1, std430) readonly buffer GlobalLightsBuffer {
    TGpuGlobalLight u_lights[MAX_GLOBAL_LIGHTS];
};

#include "Include.Pi.glsl"
#include "Include.ReconstructFragmentWorldPositionFromDepth.glsl"
#include "Include.SpecularContribution.glsl"

float GetShadow(TGpuGlobalLight light, vec3 fragmentPositionWs, float fragmentPositionZVs, vec3 normalWs) {

    int shadowCascade = MAX_SHADOW_CASCADES;
    for (int i = 0; i < MAX_SHADOW_CASCADES; i++) {
        if (fragmentPositionZVs < light.ShadowCascadeDepths[i]) {
            shadowCascade = i;
            break;
        }
    }

    vec4 fragPosLightSpace = light.LightProjectionView[shadowCascade] * vec4(fragmentPositionWs, 1.0);
    vec3 fragPosShadowSpace = fragPosLightSpace.xyz * 0.5 + 0.5;

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
    float bias = max(0.005 * (1.0 - dot(normalWs, normalize(light.Direction.xyz))), 0.0005);

    for (int i = 0; i < 16; i++) {
        shadow += 1.0 - texture(s_shadowMap, vec4(fragPosShadowSpace.xy + poissonDisk[i] / 1000.0, u_global_light_index * MAX_GLOBAL_LIGHTS + shadowCascade, fragPosShadowSpace.z - bias));
    }

    return shadow / 16.0;
}

void main() {
    float depth = texelFetch(s_texture_depth, ivec2(gl_FragCoord.xy), 0).r;
    if (depth >= 1.0) {
        discard;
    }

    vec4 albedo = texelFetch(s_texture_gbuffer_albedo, ivec2(gl_FragCoord.xy), 0);
    vec4 normal_ao = texelFetch(s_texture_gbuffer_normal_ao, ivec2(gl_FragCoord.xy), 0);
    vec4 velocity_roughness_metallic = texelFetch(s_texture_gbuffer_velocity_roughness_metallic, ivec2(gl_FragCoord.xy), 0);
    vec4 emissive_viewz = texelFetch(s_texture_gbuffer_emissive_viewz, ivec2(gl_FragCoord.xy), 0);

    float fragmentPosition_vs_z = emissive_viewz.w;
    vec3 normal = normal_ao.xyz;
    vec3 fragmentPosition_ws = ReconstructFragmentWorldPositionFromDepth(depth, u_screen_size, u_camera_inverse_view_projection);
    vec3 V = normalize(u_camera_position.xyz - fragmentPosition_ws);
    float roughness = velocity_roughness_metallic.b;
    float metallic = velocity_roughness_metallic.a;
    vec3 F0 = mix(vec3(0.04), albedo.rgb, metallic);

    TGpuGlobalLight light = u_lights[u_global_light_index];

    vec3 L = normalize(light.Direction.xyz);
    vec3 light_color = light.ColorAndIntensity.rgb * light.ColorAndIntensity.a;
    vec3 accumulated_light = SpecularContribution(L, V, normal, F0, metallic, roughness, albedo.rgb) * light_color;

    /*
    float shadow = 0.0;
    if (light.Properties.x == 1) {
        shadow = GetShadow(light, fragmentPosition_ws, fragmentPosition_vs_z, normal);
        accumulated_light += shadow;
    }
    */

    //o_accumulated_light = 0.000001 * vec4(accumulated_light, 1.0) + vec4(shadow, shadow, shadow, 1.0);
    o_accumulated_light = vec4(accumulated_light, 1.0);
}
