#version 330 core
layout (location = 0) out vec4 FragColor;

in vec4 particleColor;
in vec3 localPosition;

uniform float thickness;

void main()
{
    float x = localPosition.x;
    float y = localPosition.y;

    // outer triangle
    float x1 = (y - 1.0) / 2.0; // point on left line
    float x2 = (y - 1.0) / -2.0; // point on right line

    float d1 = x1 - x;
    float d2 = x2 - x;

    if (d1 * d2 > 0) // if both have same sign
        discard;

    FragColor = particleColor;
}
