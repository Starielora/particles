#type vertex
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec4 color;

out vec4 particleColor;

void main()
{
	gl_Position = projection * view * model * vec4(aPos, 1.0f);
	particleColor = color;
}


#type fragment
#version 330 core
out vec4 FragColor;

in vec4 particleColor;

void main()
{
    FragColor = particleColor;
}
