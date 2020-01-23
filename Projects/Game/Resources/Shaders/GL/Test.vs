#version 420

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

layout(std140) uniform MatrixBuffer
{
    mat4 world;
	mat4 view;
	mat4 proj;
} matrixBuffer;

out vec3 fragColor;

void main()
{
	vec4 pos = matrixBuffer.proj * matrixBuffer.view * matrixBuffer.world * vec4(position, 1.0);
    fragColor = color;
    gl_Position = pos;
}