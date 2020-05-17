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
    mat4 newView = mat4(mat3(scene.view)); // Remove translation.
    vec4 clipPos = scene.proj * newView * modelTransform[gl_InstanceIndex] * vec4(position.xyz, 1.0);
    gl_Position = clipPos.xyww; // Set it to max depth => z = 1.
    fragUv = position.xyz;
}