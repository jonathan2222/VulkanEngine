#version 450
#extension GL_ARB_separate_shader_objects : enable

struct Vertex
{
    vec4 inPosition;
    vec4 inNormal;
    vec4 inUv;
};

layout(set=0, binding = 0) uniform SceneData
{
    mat4 proj;
    mat4 view;
    vec4 cPos;
} scene;

layout(set=1, binding=0) uniform NodeData
{
    mat4 transform;
} node;

layout(set=0, binding = 1) readonly buffer VertexData
{
    Vertex vertices[];
};

layout(set=0, binding = 2) readonly buffer TransformData
{
    mat4 modelTransform[];
};

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragUv;
layout(location = 2) out vec3 fragPos;
layout(location = 3) out vec3 camPos;

void main() {
    gl_Position = scene.proj * scene.view * modelTransform[gl_InstanceIndex] * node.transform * vec4(vertices[gl_VertexIndex].inPosition.xyz, 1.0);
    fragPos = gl_Position.xyz;
    fragNormal = normalize((modelTransform[gl_InstanceIndex] * node.transform * vec4(vertices[gl_VertexIndex].inNormal.xyz, 0.0)).xyz);
    fragUv = vertices[gl_VertexIndex].inUv.xy;
    camPos = scene.cPos.xyz;
}