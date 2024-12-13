#version 460 core

layout(location = 1) in vec3 v_sky_ray;

layout(location = 0) out vec4 o_color;

layout(binding = 0) uniform sampler2D s_texture_gbuffer_albedo;
layout(binding = 1) uniform sampler2D s_texture_gbuffer_normal;
layout(binding = 2) uniform sampler2D s_texture_depth;
layout(binding = 8) uniform samplerCube s_texture_sky;

layout(location = 0) uniform vec3 u_sun_position;

layout (binding = 2, std140) uniform TGlobalLights {
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
    vec4 albedo = texelFetch(s_texture_gbuffer_albedo, ivec2(gl_FragCoord.xy), 0);
    vec3 normal = texelFetch(s_texture_gbuffer_normal, ivec2(gl_FragCoord.xy), 0).rgb;
    float depth = texelFetch(s_texture_depth, ivec2(gl_FragCoord.xy), 0).r;

    if (depth >= 1.0) {
        vec3 color = texture(s_texture_sky, v_sky_ray).rgb;

        o_color = vec4(color, 1.0);
        return;
    }

    vec3 reflection = normalize(reflect(v_sky_ray, normal));

    vec3 color = texture(s_texture_sky, reflection).rgb;

    float sun_n_dot_l = clamp(dot(normal, -normalize(u_sun_position)), 0.0, 1.0);

    o_color = vec4((color) + (albedo.rgb * sun_n_dot_l), 1.0);
}
