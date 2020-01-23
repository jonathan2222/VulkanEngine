#version 420

in vec2 fragUV;

out vec4 finalColor;

uniform sampler2D tex;

void main()
{
    finalColor = texture(tex, fragUV);
}