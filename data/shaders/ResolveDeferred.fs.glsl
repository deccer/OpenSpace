#version 460 core

layout(binding = 0) uniform sampler2D s_texture_gbuffer_albedo;
layout(binding = 1) uniform sampler2D s_texture_gbuffer_normal;

layout(location = 0) out vec4 o_color;

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

    float sun_n_dot_l = clamp(dot(normal, normalize(vec3(1, 10, 1))), 0.0, 1.0);

    o_color = vec4(albedo.rgb * sun_n_dot_l, 1.0);
}