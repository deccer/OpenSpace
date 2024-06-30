#ifndef VERTEXTYPES_INCLUDE_GLSL
#define VERTEXTYPES_INCLUDE_GLSL

#include "BasicTypes.include.glsl"

struct SVertexPosition
{
    SPackedVec3 Position;
};

struct SVertexNormalUvTangent
{
    uint Normal;
    uint Tangent;
    SPackedVec3 UvAndTangentSign;
};

#endif // VERTEXTYPES_INCLUDE_GLSL