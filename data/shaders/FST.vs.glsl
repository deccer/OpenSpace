#version 460 core

layout (location = 0) out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
    // 2, 0 = CCW
    // 0, 2 = CW
    vec2 position = vec2(gl_VertexID == 2, gl_VertexID == 0);
    gl_Position = vec4(position * 4.0 - 1.0, 0.0, 1.0);
}
