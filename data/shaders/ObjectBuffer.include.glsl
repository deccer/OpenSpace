#ifndef OBJECT_TYPES_INCLUDE_GLSL
#define OBJECT_TYPES_INCLUDE_GLSL

struct TObject
{
    mat4 WorldMatrix;
    ivec4 InstanceParameter;
};

layout (binding = 3, std430) restrict readonly buffer TObjectsBuffer
{
    TObject Objects[];
};

#endif // OBJECT_TYPES_INCLUDE_GLSL
