#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D scene;
uniform sampler2D bloomBlur;
uniform float factor;

void main()
{             
    vec3 sceneColor = texture(scene, TexCoords).rgb;
    vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;

    sceneColor += factor * bloomColor; // additive blending

    FragColor = vec4(sceneColor, 1.0);
}
