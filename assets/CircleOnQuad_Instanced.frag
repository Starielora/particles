#version 330 core
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

in vec4 particleColor;
in vec3 localPosition;

uniform float thickness;

void main()
{
    //float thickness = 0.5;

    float length = length(localPosition);
    if (length > 1.0 || length < thickness)
        discard;

    FragColor = particleColor;
    BrightColor = particleColor;
}
