// Headers have declarations and stuff, small projects typically have one big one, for bigger stuff
// it's typically one for every or almost every source file

// this line makes it so if the header is included more than once (say, some other header also
// includes it and a source file includes this one and that one), it doesn't explode
//
// an older way is this, basically the first time it gets included, HEADERNAME_H is defined, and
// once it's defined, the header's contents won't be evaluated on further inclusions:
// #ifndef HEADERNAME_H
// #define HEADERNAME_H 1
// ... header stuff goes here
// #endif

#pragma once

// these are standard headers, people typically include all the ones they use in
// the whole project in one header somewhere
#include <inttypes.h> // uint32_t and stuff
#include <stdarg.h> // for variadic functions (functions that take a variable number of arguments, like printf)
#include <stdbool.h> // bool and true/false
#include <stdio.h>   // printf and files
#include <stdlib.h>  // miscellaneous stuff
#include <string.h>  // string functions, also memset/memcpy (they're here because
					 // they operate on "strings of bytes")

// the key to making programs work on multiple platforms is macros the compiler defines that tell
// you if you're on windows or linux or whatever else. any platform specific code that's smaller
// than a certain amount can go in an ifdef, other stuff should really go in its own file for that
// platform
#ifdef _WIN32
// these are windows headers
#define WIN32_LEAN_AND_MEAN                                                                        \
	1                // excludes obscure stuff that doesn't matter, it
					 // matters so little i don't even know what it excludes
#include <windows.h> // has everything for windows almost
#endif

// these are other headers, people usually use quotes instead of angle brackets for non-system ones
// (they just have different search orders)

// glad is a thing that loads opengl functions from the system, because everything after 1.0 has to
// be "loaded" (get the addresses of functions) from the graphics driver, 1.0 is in opengl32.dll on
// windows and you can directly use it) it has autogenerated opengl headers generated from an xml
// version of the specification that khronos maintains
#include "glad/gl.h"
#include "glad/wgl.h"

// function declarations, basically it's the first line of the function and the extern keyword.
// although it's technically not always necessary, you should use extern

// in C specifically (you can't and don't have to in C++), when a function has no arguments, you
// should write void in the brackets, because the compiler can't assume it has no arguments if you
// don't put it, because of ancient pre-standard code

// win32.c

// for functions that are critical to the program being successful, it is common
// to exit the program from within the function when it fails rather than
// returning false or some error value, as it reduces the amount of context for
// the error that needs to be returned to the caller

// create the window where rendering happens
extern void CreateMainWindow(void);

// destroy the main window and opengl context
extern void DestroyMainWindow(void);

// create an opengl context and load opengl
extern void CreateGlContext(void);

// update the window
extern bool Update(void);

// swap the framebuffer and the backbuffer
extern void Present(void);

// get the window width
extern int32_t GetWindowWidth(void);

// get the window height
extern int32_t GetWindowHeight(void);

// misc.c

// in newer versions of C, the _Noreturn keyword lets you say a function doesn't
// return (i.e. it calls exit unconditionally or something), which lets the
// compiler optimize dead code out. most things that are only in newer versions
// of C have compiler-specific ways like __declspec or __attribute__ to get at
// them that predate the standard but have the same behaviour, but i'm leaving
// those out for brevity

// show an error message (this function works like printf)
extern void FatalError(const char* format, ...);

// read a file
extern void* LoadFile(const char* name, size_t* size);

// opengl.c

// a vertex
//
// typedef declares a new name for a type, basically with the same syntax as declaring a variable.
// it's common to typedef structs, unions, and enums so the keyword isn't needed every use (so
// instead of writing struct foo, you can write foo_t)
typedef struct Vertex
{
	float position[3];
	float colour[4];
} Vertex_t;

// create a vertex buffer
extern uint32_t CreateVertexBuffer(const Vertex_t* vertices, uint32_t vertexCount);

// indices basically are indices into a vertex buffer that form faces of a mesh
typedef uint32_t Index_t[3];

// create an index buffer
extern uint32_t CreateIndexBuffer(const Index_t* indices, uint32_t indexCount);

// create a vertex array object
extern uint32_t CreateVertexArray(uint32_t vertexBuffer, uint32_t indexBuffer);

// load and compile a shader program
extern uint32_t LoadShaders(const char* vertexName, const char* fragmentName);
