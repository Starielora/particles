#version 330 core
layout (location = 0) out vec4 FragColor;

in vec4 particleColor;
in vec3 localPosition;

uniform float thickness;

void main()
{
    float x = localPosition.x;
    float y = localPosition.y;
    float lowBound = -1 + thickness;
    float highBound = 1 - thickness;

    if (x > lowBound && x < highBound && y > lowBound && y < highBound)
        discard;

    FragColor = particleColor;
}
