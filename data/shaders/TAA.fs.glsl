#version 460 core

layout (location = 0) out vec4 o_color;

layout (binding = 0) uniform sampler2D s_current_color;
layout (binding = 1) uniform sampler2D s_history_color;
layout (binding = 2) uniform sampler2D s_velocity;
layout (binding = 3) uniform sampler2D s_depth;

layout (location = 0) uniform float u_blend_factor;
layout (location = 1) uniform float u_near_plane;
layout (location = 2) uniform float u_far_plane;

float LinearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0; // back to NDC
    return (2.0 * u_near_plane * u_far_plane) / (u_far_plane + u_near_plane - z * (u_far_plane - u_near_plane));
}

vec3 ClampHistory(vec2 uv, vec3 value) {
    vec3 minValue = vec3(99999.0);
    vec3 maxValue = vec3(-99999.0);

    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            vec3 sampleC = texelFetch(s_current_color, ivec2(uv + vec2(x, y)), 0).rgb;
            minValue = min(minValue, sampleC);
            maxValue = max(maxValue, sampleC);
        }
    }
    return clamp(value, minValue, maxValue);
}

void main() {
    ivec2 uv = ivec2(gl_FragCoord.xy);
    vec2 velocity = texelFetch(s_velocity, uv, 0).xy * 2.0 - 1.0; // map back to [-1, 1]
    ivec2 previousUv = ivec2(uv + velocity);

    vec4 current = texelFetch(s_current_color, uv, 0);
    vec4 history = texelFetch(s_history_color, previousUv, 0);

    // Depth rejection
    float currentDepth = LinearizeDepth(texelFetch(s_depth, uv, 0).r);
    float previousDepth = LinearizeDepth(texelFetch(s_depth, previousUv, 0).r);
    float differenceDepth = abs(currentDepth - previousDepth);
    float rejectionFactor = clamp(1.0 - differenceDepth * 100.0, 0.0, 1.0);

    float motionLength = length(velocity);
    float motionFactor = clamp(1.0 - motionLength * 5.0, 0.0, 1.0);

    float finalWeight = u_blend_factor * rejectionFactor * motionFactor;

    vec3 clampedHistory = ClampHistory(uv, history.rgb);

    vec3 blended = mix(current.rgb, clampedHistory, finalWeight);
    o_color = vec4(blended, 1);
}
