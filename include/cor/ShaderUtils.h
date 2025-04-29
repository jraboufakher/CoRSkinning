#pragma once
#include <string>
#include <GL/glew.h>

// Compile & link a GLSL vertex+fragment into one program.
// Returns the program ID, or 0 on error.
GLuint LoadShaders(const char* vertex_file_path,
                   const char* fragment_file_path);
