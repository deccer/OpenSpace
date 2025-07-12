vec3 CalculatePrefilteredReflection(vec3 R, float roughness) {
    const float MAX_REFLECTION_LOD = 9.0; // 512x512 BRDF LUT
    float lod = roughness * MAX_REFLECTION_LOD;
    float lodf = floor(lod);
    float lodc = ceil(lod);
    vec3 a = textureLod(s_prefiltered_radiance_environment, R, lodf).rgb;
    vec3 b = textureLod(s_prefiltered_radiance_environment, R, lodc).rgb;
    return mix(a, b, lod - lodf);
}
