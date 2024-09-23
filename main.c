#include "stuff.h"

// for function declarations in the same file, extern isn't needed (it's still allowed though)
//
// it's common to do this when you need a function in another one, but it makes more sense to order
// them so the second one is first (like when there are functions used in main, but main is kept
// first so it's one of the first things in the file
//
// additionally, making a function static makes it local to that object file (compiled but unlinked
// source file), which can let the compiler optimize more

// draw the scene
static void DrawScene(void);

// much like windows, opengl uses handles. instead of defining a custom type, opengl uses integers.

// vertex buffer (the vertices of the mesh)
static uint32_t s_vertexBuffer;
// index buffer (list of indices in the vertex buffer that form the faces of the mesh)
static uint32_t s_indexBuffer;
// vertex array object (ties together a vertex buffer, index buffer, and vertex attributes to reduce
// the amount of overhead for binding a mesh to be drawn)
static uint32_t s_vertexArray;
// shader program
static uint32_t s_shader;

// main is the entry point, argc is the number of command line arguments, argv is the arguments
int32_t main(int32_t argc, char* argv[])
{
	// It's common to separate parts of a program into functions to make it easier to follow, and
	// also putting platform specific code into a function makes avoiding ifdefs easier, which is
	// always nice

	CreateMainWindow();
	CreateGlContext();

	// clang-format off
	s_vertexBuffer = CreateVertexBuffer(
		// you can declare structs/arrays inline like this to pass them to functions more easily.
		// these vertices are in screen coordinates, you would need a math library to properly
		// transform them and project them from model space to world space to screen space. the
		// vertices get multiplied with a special transformation matrix passed into the vertex
		// shader in a uniform buffer (a way to send data to the gpu for shaders to use). the
		// consequence of them being in screen coordinates is this should-be square gets stretched
		// to be half the window's width and height.
		(Vertex_t[]){
			//  x      y      z        r     g     b     a
			{{ 0.5f,  0.5f,  0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
			{{ 0.5f, -0.5f,  0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
			{{-0.5f, -0.5f,  0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
			{{-0.5f,  0.5f,  0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
		},
		4
	);

	s_indexBuffer = CreateIndexBuffer(
		(Index_t[]) {
			{0, 1, 2}, // triangle 1
			{0, 2, 3}  // triangle 2
		},
		2
	);
	// clang-format on

	s_vertexArray = CreateVertexArray(s_vertexBuffer, s_indexBuffer);

	// shaders are programs that run on the gpu. the vertex shader acts on vertices, and the
	// fragment shader acts on groups of pixels called fragments. vertex shaders handle transforming
	// coordinate spaces, normal maps, and other stuff related to the positions of vertices, while
	// fragment shaders are usually used for lighting. there are other kinds of shaders, but i
	// barely even know what they're for.
	s_shader = LoadShaders("vertex.glsl", "fragment.glsl");

	// graphical applications typically have a function that handles window events and returns
	// false when the window is closed
	while (Update())
	{
		DrawScene(); // draws stuff
		Present();   // presenting just means putting whatever you drew onto the screen
	}

	// clean up opengl resources. these probably get deleted with the context so they could probably
	// be leaked without consequence in this case, but it's better practice to clean them up.
	glDeleteProgram(s_shader);
	glDeleteVertexArrays(1, &s_vertexArray);
	// you can handily delete multiple buffers of any type in one line like this
	glDeleteBuffers(2, (uint32_t[]){s_vertexBuffer, s_indexBuffer});

	DestroyMainWindow();

	// for main specifically, this line is implied at the end of the function
	return 0;
}

void DrawScene(void)
{
	// glClearColor and glClearDepth set the clear value for those buffers, glClear clears the
	// specified buffers
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// use the shader program
	glUseProgram(s_shader);
	// bind the vertex array
	glBindVertexArray(s_vertexArray);
	// draw the mesh
	// parameters:
	// type of face to draw
	// number of indices (should be faces * verts per face)
	// the data type of the indices
	// the offset into the index buffer
	glDrawElements(GL_TRIANGLES, 2 * 3, GL_UNSIGNED_INT, (void*)(0));
}
