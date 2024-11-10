#version 460 core

layout(location = 1) in vec3 v_sky_ray;

layout(location = 0) out vec4 o_color;

layout(binding = 0) uniform sampler2D s_texture_gbuffer_albedo;
layout(binding = 1) uniform sampler2D s_texture_gbuffer_normal;
layout(binding = 2) uniform sampler2D s_texture_depth;
layout(location = 0) uniform vec3 u_position_sun;

layout (binding = 2, std140) uniform TGlobalLights {
    mat4 ProjectionMatrix;
    mat4 ViewMatrix;
    vec4 Direction;
    vec4 Strength;
} u_global_lights[8];

layout (binding = 3, std140) uniform TAtmosphereSettings {
    vec4 SunPositionAndPlanetRadius;
    vec4 RayOriginAndSunIntensity;
    vec4 RayleighScatteringCoefficientAndAtmosphereRadius;
    float MieScatteringCoefficient;
    float MieScaleHeight;
    float MiePreferredScatteringDirection;
    float RayleighScaleHeight;
} atmosphereSettings;

float GetShadow() {
    return 1.0f;
}

#include "Atmosphere.include.glsl"
//#include "Atmosphere2.include.glsl"

void main()
{
    vec4 albedo = texelFetch(s_texture_gbuffer_albedo, ivec2(gl_FragCoord.xy), 0);
    vec3 normal = texelFetch(s_texture_gbuffer_normal, ivec2(gl_FragCoord.xy), 0).rgb;
    float depth = texelFetch(s_texture_depth, ivec2(gl_FragCoord.xy), 0).r;

    if (depth >= 1.0) {

        /*
        vec3 rayStart = atmosphereSettings.RayOriginAndSunIntensity.xyz;
        vec3 rayDir = normalize(v_sky_ray);
        float rayLength = INFINITY;

        vec2 planetIntersection = PlanetIntersection(rayStart, rayDir);
        if (planetIntersection.x > 0)
        {
            rayLength = min(rayLength, planetIntersection.x);
        }

        vec3 lightDir = atmosphereSettings.SunPositionAndPlanetRadius.xyz;
        vec3 lightColor = vec3(1, 1, 1);

        vec3 transmittance = vec3(1, 0, 0.5);
        vec3 color = IntegrateScattering(rayStart, rayDir, rayLength, lightDir, lightColor, transmittance);

        //o_color = vec4(vec3(planetIntersection, 0.0), 1.0);
        o_color = vec4(color, 1.0);
        */

        vec3 color = atmosphere(
            normalize(v_sky_ray),
            atmosphereSettings.RayOriginAndSunIntensity.xyz, // ray origin = vec3(0,6372e3,0)
            atmosphereSettings.SunPositionAndPlanetRadius.xyz, // position of the sun
            atmosphereSettings.RayOriginAndSunIntensity.w, // intensity of the sun = 22.0
            atmosphereSettings.SunPositionAndPlanetRadius.w, // radius of the planet in meters = 6371e3
            atmosphereSettings.RayleighScatteringCoefficientAndAtmosphereRadius.w, // radius of the atmosphere in meters = 6471e3
            atmosphereSettings.RayleighScatteringCoefficientAndAtmosphereRadius.xyz, // Rayleigh scattering coefficient = vec3(5.5e-6, 13.0e-6, 22.4e-6)
            atmosphereSettings.MieScatteringCoefficient, // Mie scattering coefficient = 21e-6
            atmosphereSettings.RayleighScaleHeight, // Rayleigh scale height = 8e3
            atmosphereSettings.MieScaleHeight, // Mie scale height = 1.2e3
            atmosphereSettings.MiePreferredScatteringDirection // Mie preferred scattering direction = 0.758
        );

        o_color = vec4(color, 1.0);
        return;
    }

    vec3 color = vec3(1, 1, 1);
    /*
    vec3 color = atmosphere(
        normalize(reflect(v_sky_ray, vec3(0, -1, 0))),           // normalized ray direction
        atmosphereSettings.RayOriginAndSunIntensity.xyz,               // ray origin = vec3(0,6372e3,0)
        atmosphereSettings.SunPositionAndPlanetRadius.xyz,                        // position of the sun
        atmosphereSettings.RayOriginAndSunIntensity.w,                           // intensity of the sun = 22.0
        atmosphereSettings.SunPositionAndPlanetRadius.w,                         // radius of the planet in meters = 6371e3
        atmosphereSettings.RayleighScatteringCoefficientAndAtmosphereRadius.w,                         // radius of the atmosphere in meters = 6471e3
        atmosphereSettings.RayleighScatteringCoefficientAndAtmosphereRadius.xyz, // Rayleigh scattering coefficient = vec3(5.5e-6, 13.0e-6, 22.4e-6)
        atmosphereSettings.MieScatteringCoefficient,                          // Mie scattering coefficient = 21e-6
        atmosphereSettings.RayleighScaleHeight,                            // Rayleigh scale height = 8e3
        atmosphereSettings.MieScaleHeight,                          // Mie scale height = 1.2e3
        atmosphereSettings.MiePreferredScatteringDirection                           // Mie preferred scattering direction = 0.758
    );
    */

    float sun_n_dot_l = clamp(dot(normal, normalize(vec3(1, 10, 1))), 0.0, 1.0);

    o_color = vec4((albedo.rgb * sun_n_dot_l), 1.0);
}
