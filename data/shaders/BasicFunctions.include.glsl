#ifndef BASIC_FUNCTIONS_INCLUDE_GLSL
#define BASIC_FUNCTIONS_INCLUDE_GLSL

vec3 DecodeNormal(vec2 encodedNormal) {

    encodedNormal = encodedNormal * 2.0 - 1.0;

    vec3 decodedNormal = vec3(encodedNormal.x, encodedNormal.y, 1.0 - abs(encodedNormal.x) - abs(encodedNormal.y));
    float t = clamp(-decodedNormal.z, 0.0, 1.0);

    vec2 signFlip = mix(vec2(t), vec2(-t), vec2(greaterThanEqual(encodedNormal, vec2(0.0))));
    decodedNormal.xy += signFlip;

    return normalize(decodedNormal);
}


#endif // BASIC_FUNCTIONS_INCLUDE_GLSL
