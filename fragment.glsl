#version 330 core

// this is fed in from the vertex shader's output with the same name
in vec4 vertexColour;

// the fragment shader is expected to output the fragment colour
out vec4 fragmentColour;

void main()
{
    fragmentColour = vertexColour;
}
