#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 uv_in;
uniform vec2 uv_asp;
out vec2 uv_out;
void main()
{
    gl_Position = vec4(aPos.x * uv_asp.x, aPos.y * uv_asp.y, aPos.z, 1.0);
    uv_out = vec2(uv_in.x, -uv_in.y);
}