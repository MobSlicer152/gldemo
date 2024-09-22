cmake_minimum_required(VERSION 3.8)

project(gldemo LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

# header file (#include) search directories
include_directories(${CMAKE_SOURCE_DIR} glad/include)

# compile glad as a static library
set(GLAD_SOURCES glad/src/egl.c
				 glad/src/gl.c
				 glad/src/wgl.c)
add_library(glad STATIC ${GLAD_SOURCES})

# source files for the main project
set(SOURCES main.c
			misc.c
			opengl.c
			stuff.h
			win32.c

			# this is just to include these files in the project for easy access
			vertex.glsl
			fragment.glsl
			README.txt)
# make an executable from the sources
add_executable(gldemo ${SOURCES})
# link it to glad and opengl32.lib (this is platform specific and should go in an if)
target_link_libraries(gldemo PRIVATE glad opengl32.lib)

# copy the shaders for running in the debugger
add_custom_command(TARGET gldemo POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/vertex.glsl ${CMAKE_SOURCE_DIR}/fragment.glsl $<TARGET_FILE_DIR:gldemo>)

# make gldemo run by default when you press F5 in visual studio
set_property(GLOBAL PROPERTY VS_STARTUP_PROJECT gldemo)

# set the working directory for debugging so the program can find its shaders
set_property(TARGET gldemo PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:gldemo>)
