#pragma once

#define GL_GLEXT_PROTOTYPES
#include "SDL_opengl.h"

#include <string>
#include <vector>

struct ShaderPrograms {
  GLuint regular;
  GLuint subpx;
};

ShaderPrograms LoadShaders(void);