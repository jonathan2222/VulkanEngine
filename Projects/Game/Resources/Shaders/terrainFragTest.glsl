#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragUv;
layout(location = 2) in vec3 fragPos;
layout(location = 3) in vec3 camPos;

layout(location = 0) out vec4 outColor;

void main() {
	//vec3 lightDir = normalize(vec3(0.5, 2.0, 0.5));
	//float diffuse = max(dot(lightDir, normalize(fragNormal)), 0.0);
	vec3 n = normalize(fragNormal);
    outColor = vec4(n*0.5 + 0.5, 1.0);//vec4(baseColor*diffuse, 1.0);
}