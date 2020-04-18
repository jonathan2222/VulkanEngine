#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 normal;
layout(location = 2) in vec4 texCoords;

layout(set=0, binding = 0) uniform SceneData
{
    mat4 proj;
    mat4 view;
    vec4 cPos;
} scene;

layout(set=1, binding = 0) readonly buffer TransformData
{
    mat4 modelTransform[];
};

layout(set = 1, binding = 0) uniform TerrainData
{
    mat4 transform;
};

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragUv;
layout(location = 2) out vec3 fragPos;
layout(location = 3) out vec3 camPos;

void main() {
    vec4 worldPosition = transform * vec4(position.xyz, 1.0);
    fragPos = worldPosition.xyz;
    gl_Position = scene.proj * scene.view * worldPosition;
    fragNormal = normalize((transform * vec4(normal.xyz, 0.0)).xyz);
    fragUv = texCoords.xy;
    camPos = scene.cPos.xyz;
}