float Sqr(float x) {
    return x * x;
}

vec3 Sqr(vec3 x) {
    return vec3(Sqr(x.x), Sqr(x.y), Sqr(x.z));
}
