#ifndef VERTEX_TYPES_INCLUDE_GLSL
#define VERTEX_TYPES_INCLUDE_GLSL

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

#endif // VERTEX_TYPES_INCLUDE_GLSL
