#version 460 core

layout(location = 0) uniform sampler2D s_texture;

layout(location = 0) out vec4 o_color;

void main()
{
    vec3 color = texture(s_texture, gl_FragCoord.xy).rgb;
    color = pow(color, vec3(2.2));

    o_color = vec4(pow(color, 1.0 / vec3(2.2)), 1.0);    
}