#version 460 core

layout (location = 0) out gl_PerVertex
{
    vec4 gl_Position;
};
layout (location = 1) out vec3 v_sky_ray;

layout (binding = 0, std140) uniform GlobalUniforms
{
    mat4 ProjectionMatrix;
    mat4 ViewMatrix;
    vec4 CameraPosition; // xyz = position, w = fieldOfView
    vec4 CameraDirection; // xyz = direction, w = aspectRatio
} globalUniforms;

vec3 skyray(vec2 uv, float fieldOfView, float aspectRatio)
{
    float d = 0.5 / tan(fieldOfView / 2.0);
    return vec3((uv.x - 0.5) * aspectRatio, uv.y - 0.5, -d);
}

void main()
{
    // 2, 0 = CCW
    // 0, 2 = CW
    vec2 position = vec2(gl_VertexID == 2, gl_VertexID == 0);
    gl_Position = vec4(position * 4.0 - 1.0, 0.0, 1.0);
    vec2 uv = position.xy * 2.0;
    v_sky_ray = mat3(inverse(globalUniforms.ViewMatrix)) * skyray(uv, globalUniforms.CameraPosition.w, globalUniforms.CameraDirection.w);
}
