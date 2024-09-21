cmake_minimum_required(VERSION 3.5)

project(gldemo LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

include_directories(glad/include)

set(SOURCES main.c
			stuff.h
			win32.c
			glad/src/egl.c
			glad/src/gl.c
			glad/src/wgl.c)
add_executable(gldemo ${SOURCES})

