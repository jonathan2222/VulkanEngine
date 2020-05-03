#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 position;

layout(set=0, binding = 0) uniform SceneData
{
    mat4 proj;
    mat4 view;
    vec4 cPos;
} scene;

layout(set=2, binding = 0) readonly buffer TransformData
{
    mat4 modelTransform[];
};

layout(location = 0) out vec3 fragUv;

void main() {
    vec4 worldPosition = modelTransform[gl_InstanceIndex] * vec4(position.xyz, 1.0);
    mat4 newView = scene.view;
    newView[3][0] = 0.0;
    newView[3][1] = 0.0;
    newView[3][2] = 0.0;
    gl_Position = scene.proj * newView * worldPosition;
    fragUv = position.xyz;
}