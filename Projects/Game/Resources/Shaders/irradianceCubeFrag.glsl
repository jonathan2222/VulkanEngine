#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 fragPos;
layout (location = 0) out vec4 outColor;
layout (binding = 0) uniform samplerCube samplerEnv;

layout(push_constant) uniform PushConsts {
	layout (offset = 64) float deltaPhi;
	layout (offset = 68) float deltaTheta;
} consts;

//#define PI 3.1415926535897932384626433832795
const float PI = 3.14159265359;

void main()
{
	vec3 N = normalize(fragPos);
	vec3 up = vec3(0.0, 1.0, 0.0); // This might need to be flipped.
	vec3 right = normalize(cross(up, N));
	up = cross(N, right);

	const float TWO_PI = PI * 2.0;
	const float HALF_PI = PI * 0.5;

	vec3 irradiance = vec3(0.0);
	uint sampleCount = 0u;
	for (float phi = 0.0; phi < TWO_PI; phi += consts.deltaPhi) {
		for (float theta = 0.0; theta < HALF_PI; theta += consts.deltaTheta) {
			vec3 tangentSample = vec3(sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta));
			vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;
			//vec3 tempVec = cos(phi) * right + sin(phi) * up;
			//vec3 sampleVector = cos(theta) * N + sin(theta) * tempVec;
			irradiance += texture(samplerEnv, sampleVec).rgb * cos(theta) * sin(theta);
			sampleCount++;
		}
	}
	irradiance = PI * irradiance / float(sampleCount);

	outColor = vec4(irradiance, 1.0);
}