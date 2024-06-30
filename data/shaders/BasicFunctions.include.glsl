#ifndef BASICFUNCTIONS_INCLUDE_GLSL
#define BASICFUNCTIONS_INCLUDE_GLSL

vec2 SignNotZero(vec2 v)
{
    return vec2((v.x >= 0.0) ? +1.0 : -1.0, (v.y >= 0.0) ? +1.0 : -1.0);
}

vec3 DecodeNormal(vec2 encodedNormal)
{
    vec3 decodedNormal = vec3(encodedNormal.xy, 1.0 - abs(encodedNormal.x) - abs(encodedNormal.y));
    if (decodedNormal.z < 0) {
        decodedNormal.xy = (1.0 - abs(decodedNormal.yx)) * SignNotZero(decodedNormal.xy);
    }

    return normalize(decodedNormal);
}

#endif // BASICFUNCTIONS_INCLUDE_GLSL