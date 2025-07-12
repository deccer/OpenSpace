// normal distribution function
float D_GGX(float dotNH, float roughness) {
    float alpha = roughness * roughness;
    float alphaSquared = alpha * alpha;
    float denom = dotNH * dotNH * (alphaSquared - 1.0) + 1.0;
    return (alphaSquared) / (PI * denom * denom);
}
