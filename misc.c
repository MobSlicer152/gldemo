// This file implements FatalError

#include "stuff.h"

void FatalError(const char* message, ...)
{
	// stack buffer in case calloc fails
	char buffer[1024];

	// get the arguments after message for this function
	va_list args;
	va_start(args, message);

	// vsnprintf is like printf only it goes into a string, and it takes the
	// calling function's variable arguments
	//
	// the snprintf functions tell you how many characters they would write when
	// given an empty buffer, which is useful for dynamically allocating the
	// buffer to make sure it fits
	int32_t count = vsnprintf(NULL, 0, message, args);

	// generally, it's a bad idea to allocate memory at all in an error
	// function, but this is a useful illustration of how to use vsnprintf
	//
	// calloc allocates count size-byte elements on the heap, and zeroes the
	// allocation
	//
	// the +1 is for the null terminator in the string
	char* formattedMsg = calloc(count + 1, sizeof(char));
	if (!formattedMsg)
	{
		// if the allocation fails, use the stack buffer
		formattedMsg = buffer;
	}

	// format the message into the buffer
	//
	// the size of the buffer is count + 1 if formattedMsg is on the heap,
	// otherwise it's sizeof(buffer)
	//
	// this can be checked by comparing it to buffer, because local arrays decay
	// to pointers to stack memory (static/global arrays are in the bss section)
	vsnprintf(formattedMsg, formattedMsg == buffer ? sizeof(buffer) : count + 1, message, args);

	// print the message to stderr (%s is used in case % is used in the
	// formatted message)
	fprintf(stderr, "Fatal error: %s\n", formattedMsg);

#ifdef _WIN32
	// on windows, show a message box
	MessageBoxA(NULL, formattedMsg, "Fatal error", MB_ICONERROR | MB_OK);
#endif

	// normally, you would free formattedMsg (when calloc succeeds). however, because this function
	// exits right away and this program is intended to run under normal operating systems, there is
	// no consequence for unfreed memory at the end of the program (usually, people leak memory when
	// an error happens because it doesn't matter, but don't in a successful run)

	// abort is a way to exit the program in a way that clearly indicates to the
	// runtime that a bad error happened, and on a lot of Unix-likes it often
	// causes a core dump (which is useful for debugging, since it's a copy of
	// the program's memory and cpu state at the time of the error that you can
	// inspect in a debugger)
	abort();
}
