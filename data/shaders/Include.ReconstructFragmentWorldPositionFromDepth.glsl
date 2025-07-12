vec3 ReconstructFragmentWorldPositionFromDepth(float depth, vec2 screenSize, mat4 invViewProj) {

    float z = depth * 2.0 - 1.0; // [0, 1] -> [-1, 1]
    vec2 position_cs = gl_FragCoord.xy / (screenSize - 1); // [0.5, screenSize] -> [0, 1]
    vec4 position_ndc = vec4(position_cs * 2.0 - 1.0, z, 1.0); // [0, 1] -> [-1, 1]

    // undo view + projection
    vec4 position_ws = invViewProj * position_ndc;
    position_ws /= position_ws.w;

    return position_ws.xyz;
}
