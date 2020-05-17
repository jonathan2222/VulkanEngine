#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragUVW;
layout(set=0, binding=0) uniform sampler2D equirectangularMap;

layout(location = 0) out vec4 outColor;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main() {
    vec2 uv = SampleSphericalMap(normalize(fragUVW)); // make sure to normalize localPos
    vec3 color = texture(equirectangularMap, vec2(uv.x, uv.y)).rgb;
    
    outColor = vec4(color, 1.0);
}