// to avoid ifdefs everywhere and make things in other files shorter, this file implements the
// windows versions of platform-specific stuff like creating the window and loading opengl

#include "stuff.h"

// the window procedure is a function that processes events received by the window. it runs on
// another thread, which for the most common events isn't an issue, but can have consequences.
// in general, any long task should be deferred to another thread, because handling it in the window
// procedure leads to a hang (i.e. clicking a button that downloads something should tell another
// thread to do it, not directly call the download function)
static LRESULT WindowProcedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

// this function updates the s_windowWidth and s_windowHeight variables by getting the rectangle
// for the client area (the inner part of the window, not the titlebar or borders) and calculating
// its dimensions from the coordinates of its corners
static void UpdateSize(void);

// this function uses GetProcAddress to get the addresses of functions in opengl32.dll for glad
// so it can load wgl, then uses wglGetProcAddress for non-wgl functions (this function works the
// same, but it gets them from the graphics driver in a way you can't easily replicate without it)
static GLADapiproc GetGlFunction(const char* name);

// these variables are global within this file alone, but their values can be passed anywhere and
// refer to the same window/device context/opengl context.

// handles are a common way of referring to a system resource without direct access to its contents,
// to ensure valid manipulation of it. handles can be passed around freely and still refer to the
// same resource, like a pointer.

static HMODULE s_module;      // main executable handle
static HWND s_window;         // the handle to the main window
static ATOM s_wndClass;       // window class atom
static HDC s_deviceContext;   // a device context is a thing in GDI (graphics device interface, used
							  // for basic graphics in windows) that's needed for creating an opengl
							  // context
static int32_t s_windowWidth; // the width of the window
static int32_t s_windowHeight;   // the height of the window
static bool s_windowClosed;      // set to true once the user/system requests that the window close
static HMODULE s_opengl32Module; // handle to opengl32.dll, needed for getting function addresses
static HGLRC s_glContext; // a handle to the opengl context, which is a thing tied to the window
						  // that sends opengl commands to the graphics driver and puts the results
						  // into the window

void CreateMainWindow(void)
{
	// get a handle to the main executable of the application. every process has one executable and
	// several system dlls loaded
	//
	// GetModuleHandleA returns the main executable by default, but given a name can get any other
	// module.
	s_module = GetModuleHandleA(NULL);

	// it's common to print what the program is currently doing to aid in debugging.
	// in more complex projects, it's typical to use a logging library instead of printf.
	printf("Registering window class\n");

	// a window class has things like the event handling function (window procedure), icon, cursor,
	// menu, and some default colours for windows created using it
	//
	// the = {0} initializes all the fields to 0, so there's no chance of some uninitialized field
	// having unintended meaning that causes errors
	WNDCLASSEXA wndClass = {0};
	// some structs in windows can have extra data after the normal fields, this tells it that it's
	// the normal size
	wndClass.cbSize = sizeof(WNDCLASSEXA);
	// the name of the window class, other functions use this to refer to it
	wndClass.lpszClassName = "GlDemo";
	wndClass.lpfnWndProc = WindowProcedure;
	// setting the cursor means it changes back automatically within the
	// window, otherwise it gets stuck on whatever it was when it entered
	// the window until it leaves
	//
	// the first parameter is optional, it can be a handle to a dll that has a cursor resource,
	// otherwise it loads it from the system's cursors
	wndClass.hCursor = LoadCursorA(NULL, IDC_ARROW);
	// a handle to the instance of an exe that this window class is registered to
	//
	// you shouldn't register it with a dll's instance handle
	wndClass.hInstance = s_module;

	// register the window class. RegisterClass returns an atom, which is a special value that
	// refers to the window class that you can use instead of the name most of the time. also, the
	// atom is 0 when it fails.
	//
	// the function takes a pointer to the struct in order to save memory. if you don't pass it by
	// pointer, it uses many more registers/stack space, which is less ideal.
	//
	// this can only be done once, but this function is reused in CreateGlContext, so it has to be
	// checked
	if (!s_wndClass)
	{
		s_wndClass = RegisterClassExA(&wndClass);
		if (!s_wndClass)
		{
			// GetLastError gets the last error related to windowing that happened (windows has like
			// 5 separate error systems)
			FatalError("failed to register window class: error %d!", GetLastError());
		}
	}

	printf("Creating window\n");

	// create the window. these are the parameters in order:
	// no extended style
	// the window class atom, cast to a string to avoid warnings
	// the window title
	// the window style (WS_OVERLAPPEDWINDOW is the most common and gives a title bar and draggable
	// edges)
	// x position
	// y position
	// width
	// height
	// parent window
	// menu (i've never even tried using this)
	// module to associate with (has to be the same as the one the window class is registered to)
	// parameter to pass to the first use of the window procedure, can be used to pass the
	// "this" pointer of a class when one is used to implement windowing
	//
	// the function returns null on failure
	s_window = CreateWindowExA(
		0, (LPCSTR)s_wndClass, "OpenGL Demo", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, s_module, NULL);
	if (!s_window)
	{
		FatalError("failed to create window: error %d!", GetLastError());
	}

	// get the gdi device context, it's needed for opengl
	s_deviceContext = GetWindowDC(s_window);

	// update the window size
	UpdateSize();

	printf("Created %dx%d window\n", s_windowWidth, s_windowHeight);
	s_windowClosed = false;
}

void DestroyMainWindow(void)
{
	printf("Deleting OpenGL context\n");
	wglDeleteContext(s_glContext);

	printf("Destroying window\n");
	DestroyWindow(s_window);

	// this handle is no longer valid, setting it to NULL indicates that
	s_deviceContext = NULL;
}

void CreateGlContext(void)
{
	printf("Initializing OpenGL\n");

	// in order to get the addresses of wgl functions, the address of opengl32.dll is needed
	s_opengl32Module = GetModuleHandleA("opengl32.dll");

	// wgl is a windows-specific extension to opengl, and it's used in creating the context for a
	// window

	// first, you have to create a temporary context so you can load wgl and get the right pixel
	// format for the window.

	// to make the temporary context, you need a pixel format. it doesn't have to match the window,
	// since this context is used to get the correct one and not for rendering.
	//
	// you can fill out a struct like this, or the way the window class is filled out. for constant
	// things like this, you can only fill it out this way.
	//
	// in the context of local variables, static (to the best of my knowledge) means it can't be
	// initialized at runtime, and has a value known at compile time (which can be better for
	// optimization).
	static const PIXELFORMATDESCRIPTOR TEMP_PIXEL_FORMAT = {
		// flags can be bitwise or'd together, which lets you use a
		// single integer as up to 64 flags (depending on the size of it).
		// this is way more efficient than having a bunch of booleans,
		// and is a very common pattern. some apis use a union (which
		// basically lets you access an area of memory as multiple different
		// types) of a struct with bit fields (fields that are only a subset
		// of the bits in a value) and an integer so you can use it like
		// booleans or just set flags directly like this.
		//
		// i.e.
		//
		// #define FIELD1 0b1
		// #define FIELD2 0b10
		// #define FIELD3 0b100
		//
		// union
		// {
		//     // technically, this is non-standard (not naming the struct field), but it's allowed
		//     // in most compilers
		//     struct
		//     {
		//         uint32_t field1 : 1;
		//         uint32_t field2 : 1;
		//         uint32_t field3 : 1;
		//     };
		//     uint32_t fields;
		// } x;
		//
		// x.field1 = true;
		// x.fields |= FIELD3;
		//
		// these flags mean draw into the window, allow opengl, and use a backbuffer for
		// rendering (this reduces tearing, but means they need to be swapped at the end of the
		// frame)
		.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		.iPixelType = PFD_TYPE_RGBA,
		.cColorBits = 32,
		.cDepthBits = 24,
		.cStencilBits = 8,
		0 // this means zero the rest of the fields
	};

	printf("Setting temporary pixel format\n");
	// this gets a format that matches the descriptor
	int32_t format = ChoosePixelFormat(s_deviceContext, &TEMP_PIXEL_FORMAT);
	// this sets the format for the device context
	SetPixelFormat(s_deviceContext, format, &TEMP_PIXEL_FORMAT);

	printf("Creating temporary WGL context\n");
	HGLRC tempContext = wglCreateContext(s_deviceContext);
	// note: all the examples of initializing wgl don't check the return value, but it's a good idea
	// anyway
	if (!tempContext)
	{
		FatalError("failed to create temporary WGL context: error %d!", GetLastError());
	}

	// for all opengl stuff, you need a context set as the current one
	if (!wglMakeCurrent(s_deviceContext, tempContext))
	{
		FatalError("failed to make WGL context current: error %d!", GetLastError());
	}

	// this loads wgl so other functions needed to set things up properly can be used.
	// the second parameter is a function pointer, and it expects a function that gets
	// the addresses of the functions
	gladLoadWGL(s_deviceContext, GetGlFunction);

	// attributes desired in the format. this list has an attribute for every odd index,
	// and that attribute's value for every even index. the list ends with a 0 in an odd
	// index, to indicate that there are no more attributes. this is more flexible than
	// the pixel format descriptor, and some things can only be requested in this.
	// clang-format off
	static const int32_t FORMAT_ATTRIBS[] = {
		// same as the flags from before
		WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
		WGL_DOUBLE_BUFFER_ARB, GL_TRUE,

		// same bits and pixel type as before
		WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
		WGL_COLOR_BITS_ARB, 32,
		WGL_DEPTH_BITS_ARB, 24,
		WGL_STENCIL_BITS_ARB, 8,

		// terminator
		0
	};
	// clang-format on

	int32_t goodFormat = 0;
	uint32_t formatCount = 0; // this isn't needed, but the parameter isn't optional

	// this function works like ChoosePixelFormat, but it has more knowledge of opengl
	// in order to return multiple values, it takes pointers to two integers. the first
	// one is the best format, and the second one is the number of formats it returns.
	//
	// it works like snprintf (explained in misc.c), in that if you give it 0 for the number
	// of formats it can return and null for the pointer, it just gives the number of formats
	// it would return if it could. this is a common way of giving information like that to
	// the user of the function when the amount of space can't be determined beforehand.
	wglChoosePixelFormatARB(s_deviceContext, FORMAT_ATTRIBS, NULL, 1, &goodFormat, &formatCount);

	printf("Creating real OpenGL context\n");

	// the pixel format of a window can't be set more than once. in order to set the real one in
	// case it was different is to destroy and recreate the window now.
	DestroyMainWindow();
	CreateMainWindow();

	// this list of attributes for the opengl context works the same as the pixel format attributes
	// clang-format off
	static const int32_t CONTEXT_ATTRIBS[] = {
		// the core profile has more versatility than the compatibility profile
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,

		// the last version of opengl is 4.6, from 2017
		WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
		WGL_CONTEXT_MINOR_VERSION_ARB, 6,

		// a debug context makes figuring out errors much easier
		WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,

		// terminator
		0
	};
	// clang-format on

	// set the proper pixel format for the real window. the pixel format descriptor has the
	// same basic information as the format attributes, so it's reused to describe it to windows.
	SetPixelFormat(s_deviceContext, goodFormat, &TEMP_PIXEL_FORMAT);

	// create the context
	s_glContext = wglCreateContextAttribsARB(s_deviceContext, NULL, CONTEXT_ATTRIBS);
	if (!s_glContext)
	{
		FatalError("failed to create real OpenGL context: error %d!\n", GetLastError());
	}

	// make the real one current
	wglMakeCurrent(s_deviceContext, s_glContext);

	// load the rest of opengl
	gladLoadGL(GetGlFunction);

	// glGetString is a very handy function for getting information about the OpenGL context
	printf(
		"Got %s %s OpenGL context with GLSL %s on render device %s\n", glGetString(GL_VENDOR),
		glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION),
		glGetString(GL_RENDERER));

	// show the window (it stays inaccessible and invisible to the user otherwise)
	//
	// this is done here to prevent the window from flashing in and out of existence,
	// rather than in CreateMainWindow
	ShowWindow(s_window, SW_SHOWNORMAL);
}

bool Update(void)
{
	// this is called the "message loop". in some applications, GetMessageA is used instead, but
	// it waits until a message arrives, which causes lag. Essentially, PeekMessageA gets a message
	// from the queue, TranslateMessage does some stuff with keyboard events, and DispatchMessageA
	// sends the message to the window procedure
	MSG message = {0};
	while (PeekMessageA(&message, s_window, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&message);
		DispatchMessageA(&message);
	}

	// the viewport is the area that gets rendered to, the scissor is the area of it that's visible
	glViewport(0, 0, s_windowWidth, s_windowHeight);
	glScissor(0, 0, s_windowWidth, s_windowHeight);

	return !s_windowClosed;
}

void Present(void)
{
	// this copies the backbuffer to the front (visible, drawn to the window) buffer
	wglSwapLayerBuffers(s_deviceContext, WGL_SWAP_MAIN_PLANE);
}

int32_t GetWindowWidth(void)
{
	return s_windowWidth;
}

int32_t GetWindowHeight(void)
{
	return s_windowHeight;
}

LRESULT WindowProcedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	// there are different kinds of messages a window can get, for all sorts of things like moving,
	// resizing, input, and being closed
	switch (message)
	{
	case WM_SIZE:
		UpdateSize();
		return 0;
	case WM_DESTROY:
	case WM_CLOSE:
		printf("Window closed\n");
		s_windowClosed = true;
		return 0;
	}

	// does the default behaviour for the given message, like painting the background the colour
	// specified by the window class.
	return DefWindowProcA(window, message, wparam, lparam);
}

static void UpdateSize(void)
{
	// a rect is basically the coordinates of the corners of some area
	RECT clientArea = {0};
	GetClientRect(s_window, &clientArea);
	int32_t newWidth = clientArea.right - clientArea.left;
	int32_t newHeight = clientArea.bottom - clientArea.top;

	// only print the size if it's different
	if (newWidth != s_windowWidth || newHeight != s_windowHeight)
	{
		printf(
			"Window resized from %dx%d to %dx%d\n", s_windowWidth, s_windowHeight, newWidth,
			newHeight);
	}

	s_windowWidth = newWidth;
	s_windowHeight = newHeight;
}

static GLADapiproc GetGlFunction(const char* name)
{
	// first, try getting the symbol from opengl
	PROC symbol = wglGetProcAddress(name);
	if (!symbol)
	{
		// now, try getting it from opengl32.dll
		symbol = GetProcAddress(s_opengl32Module, name);
	}

	return (GLADapiproc)symbol;
}
