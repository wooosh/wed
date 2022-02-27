#pragma once

#define GL_GLEXT_PROTOTYPES
#include "SDL_opengl.h"
#include <cstdint>

template <typename T> struct vec2 { T x, y; };

template <typename T> struct vec3 { T x, y, z; };

template <typename T> struct vec4 { T x, y, z, w; };

template <typename T> struct mat4 { T data[4][4]; };

enum ShaderAttribute {
  SHADER_SCREEN_COORD,
  SHADER_TEXTURE_COORD,
  SHADER_COLOR,
  SHADER_DEPTH,
};

static_assert(sizeof(GLshort) == sizeof(uint16_t), "assuming short is 2 bytes");
struct Vertex {
  vec2<uint16_t> screen_coord;
  vec2<uint16_t> texture_coord;
  vec4<uint8_t> color;
  uint8_t depth;
  /* TODO: merge alpha and texture coordinates on subpx?*/
};
