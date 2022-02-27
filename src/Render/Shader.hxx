#pragma once

#define GL_GLEXT_PROTOTYPES
#include "SDL_opengl.h"

struct ShaderPrograms {
  GLuint regular;
  GLuint subpx;
};

ShaderPrograms LoadShaders(void);