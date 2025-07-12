#ifndef INCLUDE_FRESNEL_GLSL
#define INCLUDE_FRESNEL_GLSL
#include "Include.Sqr.glsl"

vec3 F_Fresnel(vec3 specularColor, float VoH) {
    vec3 specularColorSqrt = sqrt(clamp(vec3(0, 0, 0), vec3(0.99, 0.99, 0.99), specularColor));
    vec3 n = (1.0 + specularColorSqrt) / (1.0 - specularColorSqrt);
    vec3 g = sqrt(n * n + VoH * VoH - 1.0);
    return 0.5 * Sqr((g - VoH) / (g + VoH)) * (1.0 + Sqr(((g + VoH) * VoH - 1) / ((g - VoH) * VoH + 1)));
}
#endif
