#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragUv;
layout(location = 1) in float fragScreenExposure;
layout(location = 2) in float fragScreenGamma;
layout(set=1, binding=0) uniform samplerCube cubemap;

layout(location = 0) out vec4 outColor;

vec3 Uncharted2Tonemap(vec3 color)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	float W = 11.2;
	return ((color*(A*color+C*B)+D*E)/(color*(A*color+B)+D*F))-E/F;
}

vec3 tonemap(vec3 color)
{
	vec3 outcol = Uncharted2Tonemap(color.rgb * fragScreenExposure);
	outcol = outcol * (1.0 / Uncharted2Tonemap(vec3(11.2)));	
	return pow(outcol, vec3(1.0 / fragScreenGamma));
}

void main() {
    vec4 color = texture(cubemap, fragUv);

    // Tone map the HDR color using Reinhard operator.
    //color.rgb = color.rgb / (color.rgb + vec3(1.0));
    // Gamma correction because the image is in linear space.
    //color.rgb = pow(color.rgb, vec3(1.0/2.2)); 

    color.rgb = tonemap(color.rgb);

    outColor = color;
}