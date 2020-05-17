#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragUv;
layout(set=1, binding=0) uniform samplerCube cubemap;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 color = texture(cubemap, fragUv);

    // Tone map the HDR color using Reinhard operator.
    color.rgb = color.rgb / (color.rgb + vec3(1.0));
    // Gamma correction because the image is in linear space.
    color.rgb = pow(color.rgb, vec3(1.0/2.2)); 

    outColor = color;
}