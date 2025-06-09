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

layout(binding = 0, std140) uniform GlobalLights {
    TGpuGlobalLight Lights[8];
} u_global_lights;

#include "Include.Pi.glsl"

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0; // Direct lighting k = (roughness + 1)^2 / 8

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
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

vec3 SamplePrefilteredEnvironmentMap(vec3 direction, float roughness) {
    const float MAX_REFLECTION_LOD = 4.0; // maxMipLevels - 1
    float lod = roughness * MAX_REFLECTION_LOD;
    return textureLod(s_prefiltered_radiance_environment, direction, lod).rgb;
}

vec3 CalculateGlobalLight(TGpuGlobalLight light, vec3 worldPos, vec3 N, vec3 V, vec3 albedo, float metallic, float roughness) {
    vec3 L = normalize(-light.Direction.xyz);
    vec3 H = normalize(V + L);

    float NdL = max(dot(N, L), 0.0);
    if (NdL <= 0.0) return vec3(0.0);

    // Calculate base reflectivity
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * NdL + 0.001;
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    return (kD * albedo / PI + specular) * light.ColorAndIntensity.rgb * light.ColorAndIntensity.w * NdL;
}

void main()
{
    float depth = texelFetch(s_texture_depth, ivec2(gl_FragCoord.xy), 0).r;
    vec4 albedo = texelFetch(s_texture_gbuffer_albedo, ivec2(gl_FragCoord.xy), 0);
    vec3 normal = texelFetch(s_texture_gbuffer_normal, ivec2(gl_FragCoord.xy), 0).rgb * 2.0 - 1.0;
    vec4 velocity_metallic_roughness = texelFetch(s_texture_gbuffer_velocity_metallic_roughness, ivec2(gl_FragCoord.xy), 0);

    if (depth >= 1.0) {
        vec3 color = texture(s_environment, v_sky_ray).rgb;

        o_color = vec4(color, 1.0);
        return;
    }

    float metallic = velocity_metallic_roughness.b;
    float roughness = velocity_metallic_roughness.a;
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo.rgb, metallic);

    vec3 fragmentPosition_ws = ReconstructFragmentWorldPositionFromDepth(depth, u_screen_size, u_camera_inverse_view_projection);
    vec3 v = normalize(u_camera_position.xyz - fragmentPosition_ws);
    vec3 n = normalize(normal);
    vec3 r = reflect(-v, n);
    vec3 f = FresnelSchlick(max(dot(n, v), 0.0), F0);
    //vec3 f = FresnelSchlickRoughness(max(dot(n, v), 0.0), F0, roughness);

    vec3 irradiance = texture(s_irradiance_environment, n).rgb;
    vec3 prefilteredColor = SamplePrefilteredEnvironmentMap(r, roughness);
    vec2 brdf = texture(s_brdf_lut, vec2(max(dot(n, v), 0.0), roughness)).rg;

    // IBL diffuse & specular
    vec3 diffuse = irradiance * albedo.rgb;
    vec3 specular = prefilteredColor * (f * brdf.x + brdf.y);

    vec3 kS = f;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;

    vec3 indirect_light = (kD * diffuse + specular); // * ao;

    vec3 direct_light = vec3(0.0);
    for (int i = 0; i < 8; i++) {
        if (u_global_lights.Lights[i].Properties.x == 1) {
            direct_light += CalculateGlobalLight(u_global_lights.Lights[i], fragmentPosition_ws, n, v, albedo.rgb, metallic, roughness);
        }
    }

    vec3 color = indirect_light + direct_light;

    o_color = vec4(color, 1.0);
}
