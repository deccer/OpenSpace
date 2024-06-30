#ifndef OBJECTTYPES_INCLUDE_GLSL
#define OBJECTTYPES_INCLUDE_GLSL

struct SObject
{
    mat4 WorldMatrix;
    ivec4 InstanceParameter;
};

layout (binding = 3, std430) restrict readonly buffer ObjectsBuffer
{
    SObject Objects[];
};

#endif // OBJECTTYPES_INCLUDE_GLSL