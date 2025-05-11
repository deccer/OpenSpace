#version 460 core

#extension GL_NV_shader_atomic_float : enable

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(binding = 0) uniform samplerCube irradianceMap;
layout(binding = 1, std430) buffer SHBuffer {
    float shR[9];
    float shG[9];
    float shB[9];
};

layout(location = 0) uniform int faceSize;

vec3 TexelToDir(uint face, vec2 uv) {
    vec3 dir;
    uv = uv * 2.0 - 1.0;
    switch (face) {
        case 0: dir = vec3( 1.0, -uv.y, -uv.x); break; // POS_X
        case 1: dir = vec3(-1.0, -uv.y,  uv.x); break; // NEG_X
        case 2: dir = vec3( uv.x,  1.0,  uv.y); break; // POS_Y
        case 3: dir = vec3( uv.x, -1.0, -uv.y); break; // NEG_Y
        case 4: dir = vec3( uv.x, -uv.y,  1.0); break; // POS_Z
        case 5: dir = vec3(-uv.x, -uv.y, -1.0); break; // NEG_Z
    }
    return normalize(dir);
}

float Area(float x, float y) {
    return atan(x * y, sqrt(x * x + y * y + 1.0));
}

float TexelSolidAngle(int x, int y, int size) {
    float u = (2.0 * (x + 0.5) / size) - 1.0;
    float v = (2.0 * (y + 0.5) / size) - 1.0;
    float invRes = 1.0 / size;
    float x0 = u - invRes;
    float y0 = v - invRes;
    float x1 = u + invRes;
    float y1 = v + invRes;

    return Area(x1, y1) - Area(x0, y1) - Area(x1, y0) + Area(x0, y0);
}

void ComputeSHBasis(vec3 dir, out float sh[9]) {
    float x = dir.x, y = dir.y, z = dir.z;
    sh[0] = 0.282095;
    sh[1] = 0.488603 * y;
    sh[2] = 0.488603 * z;
    sh[3] = 0.488603 * x;
    sh[4] = 1.092548 * x * y;
    sh[5] = 1.092548 * y * z;
    sh[6] = 0.315392 * (3.0 * z * z - 1.0);
    sh[7] = 1.092548 * x * z;
    sh[8] = 0.546274 * (x * x - y * y);
}

void main() {
    uint face = gl_WorkGroupID.z;
    uint x = gl_GlobalInvocationID.x;
    uint y = gl_GlobalInvocationID.y;
    if (x >= faceSize || y >= faceSize) return;

    vec2 uv = (vec2(x, y) + 0.5) / float(faceSize);
    vec3 dir = TexelToDir(face, uv);
    vec3 color = textureLod(irradianceMap, dir, 0.0).rgb;
    float weight = TexelSolidAngle(int(x), int(y), faceSize);

    float sh[9];
    ComputeSHBasis(dir, sh);

    for (int i = 0; i < 9; ++i) {
        atomicAdd(shR[i], color.r * sh[i] * weight);
        atomicAdd(shG[i], color.g * sh[i] * weight);
        atomicAdd(shB[i], color.b * sh[i] * weight);
    }
}
