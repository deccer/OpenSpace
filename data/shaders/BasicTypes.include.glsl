#ifndef BASICTYPES_INCLUDE_GLSL
#define BASICTYPES_INCLUDE_GLSL

struct TPackedVec2
{
    float x;
    float y;
};

struct TPackedVec3
{
    float x;
    float y;
    float z;
};

struct TPackedVec4
{
    float x;
    float y;
    float z;
    float w;
};

vec2 PackedToVec2(in TPackedVec2 v)
{
    return vec2(v.x, v.y);
}

TPackedVec2 Vec2ToPacked(in vec2 v)
{
    return TPackedVec2(v.x, v.y);
}

vec3 PackedToVec3(in TPackedVec3 v)
{
    return vec3(v.x, v.y, v.z);
}

TPackedVec3 Vec3ToPacked(in vec3 v)
{
    return TPackedVec3(v.x, v.y, v.z);
}

vec4 PackedToVec4(in TPackedVec4 v)
{
    return vec4(v.x, v.y, v.z, v.w);
}

TPackedVec4 Vec4ToPacked(in vec4 v)
{
    return TPackedVec4(v.x, v.y, v.z, v.w);
}

#endif // BASICTYPES_INCLUDE_GLSL