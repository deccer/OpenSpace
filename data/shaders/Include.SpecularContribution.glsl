#include "Include.DGgx.glsl"
#include "Include.GSchlickSmithGgx.glsl"
#include "Include.FresnelSchlick.glsl"

vec3 SpecularContribution(vec3 L, vec3 V, vec3 N, vec3 F0, float metallic, float roughness, vec3 albedo) {

    vec3 H = normalize (L + V);
    float dotNH = clamp(dot(N, H), 0.0, 1.0);
    float dotNV = clamp(dot(N, V), 0.001, 1.0);
    float dotNL = clamp(dot(N, L), 0.001, 1.0);

    vec3 color = vec3(0.0);

    if (dotNL > 0.0) {
        // D = Normal distribution (distribution of the microfacets)
        float D = D_GGX(dotNH, roughness);
        // G = Geometric shadowing term (microfacets shadowing)
        float G = G_SchlicksmithGGX(dotNL, dotNV, roughness);
        // F = Fresnel factor (Reflectance depending on angle of incidence)
        vec3 F = FresnelSchlick(dotNV, F0);
        vec3 specular = D * F * G / (4.0 * dotNL * dotNV + 0.001);
        vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
        color += (kD * albedo / PI + specular) * dotNL;
    }

    return color;
}
