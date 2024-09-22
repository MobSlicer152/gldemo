// the minimum version of the opengl spec for using this shader
#version 330 core

// the location corresponds to the vertex attribute index
layout (location = 0) in vec3 position;
layout (location = 1) in vec4 colour;

// this has to be passed along to the fragment shader
out vec4 vertexColour;

void main()
{
    gl_Position = vec4(position, 1.0);
    vertexColour = colour;
}
