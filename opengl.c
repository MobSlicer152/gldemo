#include "stuff.h"

uint32_t CreateVertexBuffer(const Vertex_t* vertices, uint32_t vertexCount)
{
	// special value to know if the buffer was created successfully, the glGen* functions don't
	// modify the value when an error happens
	uint32_t buffer = GL_INVALID_VALUE;
	// creates a buffer resource on the gpu
	glGenBuffers(1, &buffer);
	if (buffer == GL_INVALID_VALUE)
	{
		// glGetError gives an error code, like GetLastError()
		FatalError("failed to create vertex buffer: %d!", glGetError());
	}

	// resources have to be bound in order to manipulate or use them. the buffer is bound as the
	// array buffer, which is used for vertex buffers.
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	// upload the data, static draw means they don't get changed, telling the driver
	// this lets it use the optimal type of memory here
	glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(Vertex_t), vertices, GL_STATIC_DRAW);
	// unbind it by binding nothing
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// label the buffer as a vertex buffer, not super useful but narrows it down a little
	// in error/debug messages
	glObjectLabel(GL_BUFFER, buffer, 13, "Vertex buffer");

	return buffer;
}

uint32_t CreateIndexBuffer(const Index_t* indices, uint32_t indexCount)
{
	uint32_t buffer = GL_INVALID_VALUE;
	glGenBuffers(1, &buffer);
	if (buffer == GL_INVALID_VALUE)
	{
		FatalError("failed to create index buffer: %d!", glGetError());
	}

	// the element array buffer is used for indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(Index_t), indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glObjectLabel(GL_BUFFER, buffer, 12, "Index buffer");

	return buffer;
}

uint32_t CreateVertexArray(uint32_t vertexBuffer, uint32_t indexBuffer)
{
	uint32_t vertexArray = GL_INVALID_VALUE;
	glGenVertexArrays(1, &vertexArray);
	if (vertexArray == GL_INVALID_VALUE)
	{
		FatalError("failed to create vertex array: %d!", glGetError());
	}

	// while a vertex array is bound, certain bindings will attach resources to it.
	// they store the state necessary to draw things, and make binding all the resources needed at
	// once easier for the programmer and the driver.
	glBindVertexArray(vertexArray);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);

	// vertex attributes describe each part of a vertex, so the driver knows how to understand the
	// vertex data it's given. more complex renderers often have many of these for different types
	// of objects that get drawn, to make the best use of memory.
	// parameters:
	// index (basically an id for this attribute)
	// count (the number of elements in this attribute)
	// type (the type of data)
	// normalized (whether to clamp the value to [-1.0, 1.0]
	// vertex size
	// offset (into a vertex)

	// position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_t), (void*)(0));
	glEnableVertexAttribArray(0);

	// colour
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_TRUE, sizeof(Vertex_t), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// unbind the vertex array
	glBindVertexArray(0);

	return vertexArray;
}

// reads a file in and compiles it as a shader
static uint32_t LoadShader(const char* name, GLenum shaderType)
{
	printf("Loading shader %s\n", name);

	// read in the shader
	size_t shaderSize = 0;
	void* shaderData = LoadFile(name, &shaderSize);

	// create an empty shader resource
	uint32_t shader = glCreateShader(shaderType);

	// set the source for the shader
	glShaderSource(shader, 1, (const char*[]){shaderData}, (int32_t[]){(int32_t)shaderSize});

	// the shader source code isn't needed anymore, it's been given to the driver
	free(shaderData);

	// compile the shader
	glCompileShader(shader);

	// get the status of the shader
	int32_t success = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		// get the first 512 or less characters of the log (it's relatively easy to get the whole
		// thing, but that's not very important here)
		char errorLog[512] = {0};
		glGetShaderInfoLog(shader, sizeof(errorLog), NULL, errorLog);
		// delete the invalid shader
		glDeleteShader(shader);
		FatalError("failed to compile shader %s: %s\n", name, errorLog);
	}

	// label the shader for debugging
	glObjectLabel(GL_SHADER, shader, (int32_t)strlen(name), name);

	return shader;
}

uint32_t LoadShaders(const char* vertexName, const char* fragmentName)
{
	uint32_t vertexShader = LoadShader(vertexName, GL_VERTEX_SHADER);
	uint32_t fragmentShader = LoadShader(fragmentName, GL_FRAGMENT_SHADER);

	// a shader program combines multiple stages (you need at least a vertex and fragment shader 99%
	// of the time)
	uint32_t program = glCreateProgram();
	if (program == GL_INVALID_VALUE)
	{
		FatalError("failed to create shader program: %d!\n", glGetError());
	}

	// attach the shaders to the program, and link it
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	// get the link status, similar to the compile status
	int32_t success = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success)
	{
		char errorLog[512] = {0};
		glGetProgramInfoLog(program, sizeof(errorLog), NULL, errorLog);
		FatalError(
			"failed to link shader program from %s and %s: %s\n", vertexName, fragmentName,
			errorLog);
	}

	// detach and delete the shaders, the program is independant now
	glDetachShader(program, vertexShader);
	glDeleteShader(vertexShader);
	glDetachShader(program, fragmentShader);
	glDeleteShader(fragmentShader);

	return program;
}
