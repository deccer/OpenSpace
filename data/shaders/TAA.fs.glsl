#version 460 core

layout (binding = 0) uniform sampler2D s_current_color;
layout (binding = 1) uniform sampler2D s_history_color;
layout (binding = 2) uniform sampler2D s_velocity;
layout (binding = 3) uniform sampler2D s_depth;

layout (location = 4) uniform float u_blend_factor;
layout (location = 5) uniform vec2 u_resolution;

layout (location = 0) out vec4 o_color;

void main() {
    ivec2 uv = ivec2(gl_FragCoord.xy) / textureSize(s_current_color, 0);
    vec2 velocity = texelFetch(s_velocity, uv, 0).xy * 2.0 - 1.0; // map back to [-1, 1]
    ivec2 previousUv = ivec2(uv + velocity / u_resolution);

    previousUv = clamp(previousUv, ivec2(0.0), ivec2(u_resolution));

    vec4 current = texelFetch(s_current_color, uv, 0);
    vec4 history = texelFetch(s_history_color, previousUv, 0);

    o_color = mix(current, history, u_blend_factor);
    //o_color = vec4(uv, previousUv);
}
