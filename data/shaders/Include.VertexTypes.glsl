#ifndef VERTEX_TYPES_INCLUDE_GLSL
#define VERTEX_TYPES_INCLUDE_GLSL

#include "BasicTypes.include.glsl"

struct TVertexPosition
{
    TPackedVec3 Position;
};

struct TPackedVertexNormalTangentUvSign
{
    uint Normal;
    uint Tangent;
    TPackedVec4 UvAndTangentSign;
};

struct TVertexNormalTangentUvSign
{
    vec4 Normal;
    vec4 Tangent;
    vec4 UvAndTangentSign;
};

#endif // VERTEX_TYPES_INCLUDE_GLSL
