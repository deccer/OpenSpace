#version 460 core

layout(location = 1) in vec3 v_sky_ray;

layout(location = 0) out vec4 o_color;

layout(binding = 0) uniform sampler2D s_texture_gbuffer_albedo;
layout(binding = 1) uniform sampler2D s_texture_gbuffer_normal;
layout(binding = 2) uniform sampler2D s_texture_depth;

layout(binding = 8) uniform samplerCube s_texture_sky;
layout(binding = 9) uniform samplerCube s_convolved_sky;

layout(location = 0) uniform vec3 u_sun_position;
layout(location = 1) uniform vec4 u_camera_position;
layout(location = 2) uniform mat4 u_camera_inverse_view_projection;
layout(location = 3) uniform vec2 u_screen_size;

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

void main()
{
    vec4 albedo = texelFetch(s_texture_gbuffer_albedo, ivec2(gl_FragCoord.xy), 0);
    vec3 normal = texelFetch(s_texture_gbuffer_normal, ivec2(gl_FragCoord.xy), 0).rgb * 2.0 - 1.0;
    float depth = texelFetch(s_texture_depth, ivec2(gl_FragCoord.xy), 0).r;

    vec3 fragmentPosition_ws = ReconstructFragmentWorldPositionFromDepth(depth, u_screen_size, u_camera_inverse_view_projection);
    vec3 v = normalize(u_camera_position.xyz - fragmentPosition_ws);

    if (depth >= 1.0) {
        vec3 color = texture(s_texture_sky, v_sky_ray).rgb;

        o_color = vec4(color, 1.0);
        return;
    }

    float sun_n_dot_l = clamp(dot(normal, normalize(u_sun_position)), 0.001, 0.999);

    float metallic = 0.25;
    float roughness = 0.5;
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo.rgb, metallic);
    vec3 kS = FresnelSchlickRoughness(max(dot(normal, v), 0.0), F0, roughness);
    vec3 kD = 1.0 - kS;
    float ao = 1.0;
    vec3 irradiance = texture(s_convolved_sky, normalize(normal)).rgb;
    vec3 diffuse = irradiance * albedo.rgb;
    vec3 ambient = (kD * diffuse) * ao;

    o_color = vec4(ambient * sun_n_dot_l, 1.0);
}
