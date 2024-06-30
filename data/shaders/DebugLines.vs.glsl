#version 460 core

layout(location = 0) in vec3 i_position;
layout(location = 1) in vec4 i_color;

layout (location = 0) out gl_PerVertex
{
    vec4 gl_Position;
};
layout(location = 0) out vec4 v_color;

layout (location = 0, std140) uniform CameraInformation
{
    mat4 ProjectionMatrix;
    mat4 ViewMatrix;
    vec4 CameraPosition;
} u_camera_information;

void main()
{
    v_color = i_color;
    gl_Position = u_camera_information.ProjectionMatrix * 
                  u_camera_information.ViewMatrix * 
                  vec4(i_position, 1.0);
}
