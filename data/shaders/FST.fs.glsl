#version 460 core

layout(location = 0) out vec4 o_color;

layout(location = 0) uniform sampler2D s_texture;

layout(location = 1) uniform int u_enable_fxaa;

#include "FXAA.include.glsl"

void main()
{
    if (u_enable_fxaa == 1) {
        o_color = textureFXAA(s_texture, vec2(gl_FragCoord.xy) / textureSize(s_texture, 0));
    } else {
        o_color = texture(s_texture, gl_FragCoord.xy);
    }
}
