#version 330 core
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

in vec4 particleColor;
in vec3 localPosition;

void main()
{
    FragColor = particleColor;
    BrightColor = particleColor;
}
