// to avoid ifdefs everywhere and make things in other files shorter, this file implements the
// windows versions of platform-specific stuff like creating the window and loading opengl

#include "stuff.h"

// the window procedure is a function that processes events received by the window. it runs on
// another thread, which for the most common events isn't an issue, but can have consequences.
// in general, any long task should be deferred to another thread, because handling it in the window
// procedure leads to a hang (i.e. clicking a button that downloads something should tell another
// thread to do it, not directly call the download function)
static LRESULT WindowProcedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

// these variables are global within this file alone, but their values can be passed anywhere and
// refer to the same window/device context/opengl context

// handles are a common way of referring to a system resource without direct access to its contents,
// to ensure valid manipulation of it. handles can be passed around freely and still refer to the
// same resource, like a pointer.

static HMODULE s_module;    // main executable handle
static HWND s_window;       // the handle to the main window
static ATOM s_wndClass;     // window class atom
static HDC s_deviceContext; // a device context is a thing in GDI (graphics device interface) that's
							// needed for creating an opengl context
static int32_t s_windowWidth;
static int32_t s_windowHeight;

void CreateMainWindow(void)
{
	// get a handle to the main executable of the application. every process has one executable
	// loaded, and several system dlls
	s_module = GetModuleHandleA(
		NULL); // takes the name of the module (exe/dll) to get, default is the main exe

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
	s_wndClass = RegisterClassExA(&wndClass);
	if (!s_wndClass)
	{
		// GetLastError gets the last error related to windowing that happened (windows has like 5
		// separate error systems)
		FatalError("failed to register window class: error %d!", GetLastError());
	}

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

	// show the window (it stays inaccessible otherwise)
	ShowWindow(s_window, SW_SHOWNORMAL);
}

void CreateGlContext(void)
{
}

bool Update(void)
{
	return true;
}

void Present(void)
{
}

LRESULT WindowProcedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	// does the default behaviour for the given message, like painting the background the colour
	// specified by the window class.
	return DefWindowProcA(window, message, wparam, lparam);
}
