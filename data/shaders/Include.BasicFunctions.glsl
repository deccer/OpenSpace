#ifndef BASIC_FUNCTIONS_INCLUDE_GLSL
#define BASIC_FUNCTIONS_INCLUDE_GLSL

vec3 DecodeOctahedral(vec2 e) {
    vec3 v = vec3(e.xy, 1.0 - abs(e.x) - abs(e.y));
    vec2 signNotZero = vec2((v.x >= 0.0) ? +1.0 : -1.0, (v.y >= 0.0) ? +1.0 : -1.0);
    if (v.z < 0.0) v.xy = (1.0 - abs(v.yx)) * signNotZero;
    return normalize(v);
}

vec2 EncodeOctahedral(vec3 v) {
    vec2 p = vec2(v.x, v.y) * (1.0 / (abs(v.x) + abs(v.y) + abs(v.z)));
    vec2 signNotZero = vec2((p.x >= 0.0) ? 1.0 : -1.0, (p.y >= 0.0) ? 1.0 : -1.0);
    return (v.z <= 0.0) ? ((1.0 - abs(vec2(p.y, p.x))) * signNotZero) : p;
}

#endif // BASIC_FUNCTIONS_INCLUDE_GLSL
