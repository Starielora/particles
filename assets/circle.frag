#version 330 core
layout (location = 0) out vec4 FragColor;

in vec4 particleColor;
in vec3 localPosition;

uniform float thickness;

void main()
{
    float length = length(localPosition);
    if (length < (1 - thickness) || length > 1.0)
        discard;

    FragColor = particleColor;
}
