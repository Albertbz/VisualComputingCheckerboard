// Simple loader header for ImGui's OpenGL3 backend.
// This forwards to whichever OpenGL loader is available/selected (glad,
// glew, gl3w, glbinding). If your project uses a specific loader, define one
// of IMGUI_IMPL_OPENGL_LOADER_GLAD, IMGUI_IMPL_OPENGL_LOADER_GLEW,
// IMGUI_IMPL_OPENGL_LOADER_GL3W, IMGUI_IMPL_OPENGL_LOADER_GLBINDING2 or
// IMGUI_IMPL_OPENGL_LOADER_GLBINDING3 before including ImGui backends.

#ifndef IMGUI_IMPL_OPENGL3_LOADER_H
#define IMGUI_IMPL_OPENGL3_LOADER_H

#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
#include <GL/gl3w.h>
#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
#include <GL/glew.h>
#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
#include <glad/glad.h>
#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING2)
#include <glbinding/gl/gl.h>
using namespace gl;
#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING3)
#include <glbinding/glbinding.h>
#include <glbinding/gl/gl.h>
using namespace gl;
#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#else
// Try to auto-detect common loader headers
#if __has_include(<glad/glad.h>)
#include <glad/glad.h>
#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#elif __has_include(<GL/glew.h>)
#include <GL/glew.h>
#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#elif __has_include(<GL/gl3w.h>)
#include <GL/gl3w.h>
#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#else
#error                                                                         \
    "No OpenGL loader found. Define IMGUI_IMPL_OPENGL_LOADER_GLAD (or other) or install glad/glew/gl3w."
#endif
#endif

#endif // IMGUI_IMPL_OPENGL3_LOADER_H
