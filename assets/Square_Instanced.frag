#version 330 core
out vec4 FragColor;

in vec4 particleColor;
in vec3 localPosition;

void main()
{
    FragColor = particleColor;
}
