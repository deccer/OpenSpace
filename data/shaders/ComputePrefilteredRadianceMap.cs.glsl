#version 460 core

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(binding = 0) uniform samplerCube s_environment_map;
layout(binding = 1, rgba16f) restrict writeonly uniform imageCube o_prefiltered_map;

layout(location = 0) uniform float u_roughness;
layout(location = 1) uniform int u_mipLevel;
layout(location = 2) uniform int u_size;

const float PI = 3.14159265359;

#include "Include.Hammersley.glsl"

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness) {
    float a = roughness * roughness;
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);

    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}

vec3 GetCubemapVector(ivec3 coords, uint size) {
    vec2 uv = vec2(coords.xy) / float(size) * 2.0 - 1.0;
    switch(coords.z) {
        case 0: return normalize(vec3(1.0, -uv.y, -uv.x));  // Positive X
        case 1: return normalize(vec3(-1.0, -uv.y, uv.x));  // Negative X
        case 2: return normalize(vec3(uv.x, 1.0, uv.y));    // Positive Y
        case 3: return normalize(vec3(uv.x, -1.0, -uv.y));  // Negative Y
        case 4: return normalize(vec3(uv.x, -uv.y, 1.0));   // Positive Z
        case 5: return normalize(vec3(-uv.x, -uv.y, -1.0)); // Negative Z
        default: return vec3(0.0);
    }
}

void main() {
    if (any(greaterThanEqual(gl_GlobalInvocationID.xyz, uvec3(u_size, u_size, 6)))) {
        return;
    }

    vec3 N = GetCubemapVector(ivec3(gl_GlobalInvocationID), u_size);
    vec3 R = N;
    vec3 V = R;

    const uint SAMPLE_COUNT = 1024u;
    vec3 prefilteredColor = vec3(0.0);
    float totalWeight = 0.0;

    for(uint i = 0u; i < SAMPLE_COUNT; ++i) {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampleGGX(Xi, N, u_roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if(NdotL > 0.0) {
            prefilteredColor += textureLod(s_environment_map, L, 0.0).rgb * NdotL;
            totalWeight += NdotL;
        }
    }

    prefilteredColor = prefilteredColor / totalWeight;
    imageStore(o_prefiltered_map, ivec3(gl_GlobalInvocationID), vec4(prefilteredColor, 1.0));
}
