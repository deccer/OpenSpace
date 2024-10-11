#ifndef VERTEXTYPES_INCLUDE_GLSL
#define VERTEXTYPES_INCLUDE_GLSL

#include "BasicTypes.include.glsl"

struct TVertexPosition
{
    TPackedVec3 Position;
};

struct TVertexNormalTangentUvSign
{
    uint Normal;
    uint Tangent;
    TPackedVec4 UvAndTangentSign;
};

#endif // VERTEXTYPES_INCLUDE_GLSL