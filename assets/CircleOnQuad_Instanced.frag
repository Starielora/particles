#version 330 core
out vec4 FragColor;

in vec4 particleColor;
in vec3 localPosition;

void main()
{
    float thickness = 0.5;

    float length = length(localPosition);
    if (length > 1.0 || length < thickness)
        discard;

    FragColor = particleColor;
}
