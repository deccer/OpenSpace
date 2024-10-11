#version 460 core

layout(location = 0) uniform sampler2D s_texture;

layout(location = 0) out vec4 o_color;

void main()
{
    o_color = texture(s_texture, gl_FragCoord.xy);
}