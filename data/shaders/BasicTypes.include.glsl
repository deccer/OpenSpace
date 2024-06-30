#ifndef BASICTYPES_INCLUDE_GLSL
#define BASICTYPES_INCLUDE_GLSL

struct SPackedVec2
{
    float x;
    float y;
};

struct SPackedVec3
{
    float x;
    float y;
    float z;
};

struct SPackedVec4
{
    float x;
    float y;
    float z;
    float w;
};

vec2 PackedToVec2(in SPackedVec2 v)
{
    return vec2(v.x, v.y);
}

SPackedVec2 Vec2ToPacked(in vec2 v)
{
    return SPackedVec2(v.x, v.y);
}

vec3 PackedToVec3(in SPackedVec3 v)
{
    return vec3(v.x, v.y, v.z);
}

SPackedVec3 Vec3ToPacked(in vec3 v)
{
    return SPackedVec3(v.x, v.y, v.z);
}

vec4 PackedToVec4(in SPackedVec4 v)
{
    return vec4(v.x, v.y, v.z, v.w);
}

SPackedVec4 Vec4ToPacked(in vec4 v)
{
    return SPackedVec4(v.x, v.y, v.z, v.w);
}

#endif // BASICTYPES_INCLUDE_GLSL