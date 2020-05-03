#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragUv;
layout(set=1, binding=0) uniform samplerCube cubemap;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 color = texture(cubemap, fragUv);
    outColor = color;
}