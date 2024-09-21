#include "stuff.h"

// for function declarations in the same file, you don't need extern (it's still
// allowed though)
//
// it's common to do this when you need a function in another one, but it makes
// more sense to order them so the second one is first (like when you have
// functions you use in main, but want main first so it's one of the first
// things in the file

// draw the scene
void DrawScene(void);

// main is the entry point, argc is the number of command line arguments, argv
// is the arguments
int32_t main(int32_t argc, char* argv[])
{
	// Some functions are named how they are cause windows has functions like
	// CreateWindow, SwapBuffers, etc

	// It's common to separate parts of a program into functions to make it
	// easier to follow, and also putting platform specific code into a function
	// makes avoiding ifdefs easier, which is always nice

	CreateMainWindow();
	CreateGlContext();

	// graphical applications typically have a function that handles window
	// events and returns false when the window is closed
	while (Update())
	{
		DrawScene(); // draws stuff
		Present();   // presenting just means putting whatever you drew onto
					 // the screen
	}

	// for main specifically, this line is implied at the end of the function
	return 0;
}

void DrawScene(void)
{
	// ClearColor and ClearDepth set the clear value for that buffer, glClear
	// clears the specified buffers
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
