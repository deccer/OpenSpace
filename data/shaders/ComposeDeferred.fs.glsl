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

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
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

    vec3 fragmentPosition_ws = ReconstructFragmentWorldPositionFromDepth(depth, u_screen_size, u_camera_inverse_view_projection);
    vec3 v = normalize(u_camera_position.xyz - fragmentPosition_ws);
    vec3 n = normalize(normal);
    vec3 r = reflect(-v, n);

    float metallic = velocity_metallic_roughness.b;
    float roughness = velocity_metallic_roughness.a;

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo.rgb, metallic);

    vec3 prefilteredColor = SamplePrefilteredEnvironmentMap(r, roughness);
    vec2 brdf = texture(s_brdf_lut, vec2(max(dot(n, v), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F0 * brdf.x + brdf.y);

    vec3 kS = FresnelSchlickRoughness(max(dot(n, v), 0.0), F0, roughness);
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;

    vec3 irradiance = texture(s_irradiance_environment, n).rgb;
    vec3 diffuse = irradiance * albedo.rgb;
    vec3 ambient = (kD * diffuse + specular); // * ao

    float sun_n_dot_l = clamp(dot(n, normalize(u_sun_position)), 0.001, 0.999);

    o_color = vec4(0.0000001 * (ambient * sun_n_dot_l) + r, 1.0);
}
