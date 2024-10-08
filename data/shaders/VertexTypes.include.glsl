#ifndef VERTEXTYPES_INCLUDE_GLSL
#define VERTEXTYPES_INCLUDE_GLSL

#include "BasicTypes.include.glsl"

struct TVertexPosition
{
    TPackedVec3 Position;
};

struct TVertexNormalUvTangent
{
    uint Normal;
    uint Tangent;
    TPackedVec3 UvAndTangentSign;
};

#endif // VERTEXTYPES_INCLUDE_GLSL