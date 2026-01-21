#version 460 core

layout(location = 1) in vec3 v_sky_ray;

layout(location = 0) out vec4 o_color;

layout(binding = 0) uniform sampler2D s_texture_depth;
layout(binding = 1) uniform sampler2D s_texture_gbuffer_albedo;
layout(binding = 2) uniform sampler2D s_texture_gbuffer_normal_ao;
layout(binding = 3) uniform sampler2D s_texture_gbuffer_velocity_roughness_metallic;
layout(binding = 4) uniform sampler2D s_texture_gbuffer_emissive;

layout(binding = 5) uniform sampler2D s_shadow_maps[8];

layout(binding = 13) uniform samplerCube s_environment;
layout(binding = 14) uniform samplerCube s_irradiance_environment;
layout(binding = 15) uniform samplerCube s_prefiltered_radiance_environment;
layout(binding = 16) uniform sampler2D s_brdf_lut;

layout(location = 0) uniform mat4 u_camera_inverse_view_projection;
layout(location = 4) uniform vec4 u_camera_position;
layout(location = 5) uniform vec3 u_sun_position;
layout(location = 6) uniform vec2 u_screen_size;

struct TGpuGlobalLight {
    mat4 ShadowViewProjectionMatrix;
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

float sqr(float x) { return x*x; }
vec3 sqr(vec3 x) { return vec3(sqr(x.x), sqr(x.y), sqr(x.z)); }

vec3 F_Fresnel(vec3 SpecularColor, float VoH) {
	vec3 SpecularColorSqrt = sqrt(clamp(vec3(0, 0, 0), vec3(0.99, 0.99, 0.99), SpecularColor));
	vec3 n = (1.0 + SpecularColorSqrt) / (1.0 - SpecularColorSqrt);
	vec3 g = sqrt(n*n + VoH*VoH - 1.0);
	return 0.5 * sqr((g - VoH) / (g + VoH)) * (1.0 + sqr(((g + VoH) * VoH - 1) / ((g-VoH)*VoH + 1)));
}

vec3 CalculateDirectLighting(vec3 L, vec3 V, vec3 N, vec3 F0, float metallic, float roughness, vec3 albedo, vec3 lightColor) {

    vec3 H = normalize(L + V);

    float NdotH = max(dot(N, H), 0.0);
    float NdotV = max(dot(N, V), 0.0001);
    float NdotL = max(dot(N, L), 0.0);
    float VdotH = max(dot(V, H), 0.0);

    if (NdotL <= 0.0) {
        return vec3(0.0);
    }

    // cook torranceisms
    float D = D_GGX(NdotH, roughness); // normal Distribution
    float G = G_SchlicksmithGGX(NdotL, NdotV, roughness); // Geometry micro shadowing
    vec3 F = FresnelSchlick(VdotH, F0);

    // specular term: DFG / (4 * NdotV * NdotL)
    vec3 specular = (D * F * G) / (4.0 * NdotV * NdotL + 0.0001);

    // diffuse term: using Lambertian BRDF
    // kD is the ratio of light that gets refracted (not reflected)
    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
    vec3 diffuse = kD * albedo / PI;

    return (diffuse + specular) * lightColor * NdotL;
}

float ggx(float NdotH, float a2)
{
    float denom = (NdotH * NdotH * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}

vec3 schlick(vec3 F0, float VdotH)
{
    return F0 + (vec3(1.0) - F0) * pow(1.0 - VdotH, 5.0);
}

vec3 lighting_ggx(vec3 l_color, vec3 F0, vec3 light, vec3 eye, vec3 normal, vec3 albedo, float metallic, float alpha, float alpha2, float k2, float attenuation)
{
    vec3 halfvec = normalize(light + eye);
    float LdotH = clamp(dot(light, halfvec), 0.0, 1.0);
    float NdotH = clamp(dot(normal, halfvec), 0.0, 1.0);
    float NdotL = clamp(dot(normal, light), 0.0, 1.0);

    float D = ggx(NdotH, alpha2);
    vec3 F = schlick(F0, LdotH);

    float V = 1.0 / (LdotH * LdotH * (1.0 - k2) + k2);

    vec3 specular = F * (NdotL * D * V);

    vec3 kd = (vec3(1.0) - F) * (1.0 - metallic);

    return (attenuation * NdotL) * l_color * (kd * albedo + specular);
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

float CalculateShadow(int lightIndex, vec3 worldPosition, vec3 normal, vec3 lightDir) {
    // Transform world position to light space
    vec4 lightSpacePos = u_global_lights.Lights[lightIndex].ShadowViewProjectionMatrix * vec4(worldPosition, 1.0);

    // Perspective divide
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;

    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    // Flip Y coordinate for OpenGL texture coordinates
    projCoords.y = 1.0 - projCoords.y;

    // Outside shadow map bounds = not in shadow
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 1.0;
    }

    // Get depth from shadow map
    float closestDepth = texture(s_shadow_maps[lightIndex], projCoords.xy).r;
    float currentDepth = projCoords.z;

    // Bias to reduce shadow acne
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);

    // PCF (Percentage Closer Filtering) for soft shadows
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(s_shadow_maps[lightIndex], 0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(s_shadow_maps[lightIndex], projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (currentDepth - bias) > pcfDepth ? 0.0 : 1.0;
        }
    }
    shadow /= 9.0;

    return shadow;
}

void main()
{
    ivec2 uv_ss = ivec2(gl_FragCoord.xy);
    float depth = texelFetch(s_texture_depth, uv_ss, 0).r;

    // sky
    if (depth >= 1.0) {
        o_color = vec4(texture(s_environment, v_sky_ray).rgb, 1.0);
        return;
    }

    vec4 albedo = texelFetch(s_texture_gbuffer_albedo, uv_ss, 0);
    vec4 normal_ao = texelFetch(s_texture_gbuffer_normal_ao, uv_ss, 0);
    vec4 velocity_roughness_metallic = texelFetch(s_texture_gbuffer_velocity_roughness_metallic, uv_ss, 0);
    vec3 emissive = texelFetch(s_texture_gbuffer_emissive, uv_ss, 0).rgb;

    float roughness = velocity_roughness_metallic.b;
    float metallic = velocity_roughness_metallic.a;

    vec3 fragmentPosition_ws = ReconstructFragmentWorldPositionFromDepth(depth, u_screen_size, u_camera_inverse_view_projection);
    vec3 V = normalize(u_camera_position.xyz - fragmentPosition_ws);
    vec3 N = normalize(normal_ao.xyz);
    vec3 R = reflect(-V, N);

    float alpha     = roughness * roughness;
    float alpha2    = alpha * alpha;

    float k = 0.5 * alpha;
    float k2 = k * k;

    // base reflectance
    vec3 F0 = mix(vec3(0.04), albedo.rgb, metallic);

    // direct lighting
    vec3 directLighting = vec3(0.0);
    for (int i = 0; i < 8; i++) {
        TGpuGlobalLight light = u_global_lights.Lights[i];
        if (light.Properties.x == 1) {
            vec3 L = normalize(-light.Direction.xyz);
            vec3 lightColor = light.ColorAndIntensity.rgb * light.ColorAndIntensity.a;

            // Calculate shadow if light casts shadows
            float shadowFactor = 1.0;
            if (light.Properties.y == 1) {
                shadowFactor = CalculateShadow(i, fragmentPosition_ws, N, L);
            }

            // vec3 lighting_ggx(vec3 l_color, vec3 F0, vec3 light, vec3 eye, vec3 normal, vec3 albedo,
            // float metallic, float alpha, float alpha2, float k2, float attenuation)

            directLighting += lighting_ggx(lightColor, F0, L, V, N, albedo.rgb, metallic, alpha, alpha2, k2, 0.5) * shadowFactor;
        }
    }

    // IBL
    float NdotV = max(dot(N, V), 0.0);

    vec3 irradiance = texture(s_irradiance_environment, N).rgb;
    vec3 prefilteredColor = GetPrefilteredReflection(R, roughness);
    vec2 brdfLUT = texture(s_brdf_lut, vec2(NdotV, roughness)).rg;

    vec3 F_ambient = FresnelSchlickRoughness(NdotV, F0, roughness);

    // diffuse ambient - energy that gets refracted
    vec3 kD_ambient = (1.0 - F_ambient) * (1.0 - metallic);
    vec3 ambientDiffuse = kD_ambient * irradiance * albedo.rgb;

    // dpecular ambient - split-sum approximation
    vec3 ambientSpecular = prefilteredColor * (F_ambient * brdfLUT.x + brdfLUT.y);

    float ambientOcclusion = 1.0f;//normal_ao.w;
    vec3 ambient = (ambientDiffuse + ambientSpecular) * ambientOcclusion;

    vec3 color = ambient + directLighting + emissive;

    // Debug: Check for degenerate cases
    if (any(isnan(N)) || any(isinf(N))) {
        color = vec3(1.0, 0.0, 1.0); // Magenta for NaN/Inf normals
    }
    if (any(isnan(color)) || any(isinf(color))) {
        color = vec3(1.0, 1.0, 0.0); // Yellow for NaN/Inf color
    }

    o_color = vec4(color, 1.0);

    // o_color = vec4(ambientDiffuse, 1.0);           // Diffuse IBL
    // o_color = vec4(ambientSpecular, 1.0);          // Specular IBL (should show env on metallic)
    // o_color = vec4(prefilteredColor, 1.0);         // Raw environment reflection
    // o_color = vec4(irradiance, 1.0);               // Irradiance map
    // o_color = vec4(F_ambient, 1.0);                // Fresnel term
    // o_color = vec4(vec3(metallic), 1.0);           // Metallic value
    // o_color = vec4(vec3(roughness), 1.0);          // Roughness value
    // o_color = vec4(brdfLUT.x, brdfLUT.y, 0.0, 1.0); // BRDF LUT
    // o_color = vec4(N * 0.5 + 0.5, 1.0);            // Normals
    // o_color = vec4(R * 0.5 + 0.5, 1.0);            // Reflection vector
    // o_color = vec4(texture(s_environment, R).rgb, 1.0); // Source environment map via reflection
}
