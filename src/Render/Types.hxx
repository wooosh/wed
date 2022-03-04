#pragma once

#include <cstddef>
#include <cstdint>
#include <sys/types.h>
#include <type_traits>

#define GL_GLEXT_PROTOTYPES
#include "SDL_opengl.h"

/* TODO: move to Util */

template <typename T> T DivideRoundUp(T lhs, T rhs) {
  static_assert(std::is_integral_v<T>);
  return lhs / rhs + (lhs % rhs != 0);
}

template <typename T> struct vec2 { T x, y; };

template <typename T> struct vec3 { T x, y, z; };

template <typename T> struct vec4 { T x, y, z, w; };

template <typename T> struct mat4 { T data[4][4]; };

struct Point {
  uint x, y;
};

struct Rect {
  uint x, y, w, h;
  Point top_left() const { return {x, y}; };
};

struct Color {
  uint8_t r, g, b, a;
  operator vec4<uint8_t>() const { return {r, g, b, a}; }
};

constexpr Color Dim(float f) { return {0, 0, 0, (uint8_t)(f * 255)}; }

constexpr Color RGBA(uint32_t lit) {
  return {(uint8_t)(lit >> 24), (uint8_t)(lit >> 16), (uint8_t)(lit >> 8),
          (uint8_t)(lit)};
}

constexpr Color RGB(uint32_t lit) {
  return {(uint8_t)(lit >> 16), (uint8_t)(lit >> 8), (uint8_t)(lit), 255};
}

struct GPUTexture {
  GLuint id;
  vec2<uint> size;
  enum class Format { kRGB, kRGBA, kGrayscale } format;
};

enum ShaderAttribute {
  SHADER_SCREEN_COORD,
  SHADER_TEXTURE_COORD,
  SHADER_COLOR,
  SHADER_DEPTH,
};

typedef uint8_t RenderLayerIdx;
#define RENDER_LAYER_IDX_MAX UINT8_MAX
#define RENDER_LAYER_IDX_GLTYPE GL_UNSIGNED_BYTE

static_assert(sizeof(GLshort) == sizeof(uint16_t), "assuming short is 2 bytes");
struct Vertex {
  vec2<uint16_t> screen_coord;
  vec2<uint16_t> texture_coord;
  vec4<uint8_t> color;
  RenderLayerIdx depth;
  /* TODO: merge alpha and texture coordinates on subpx?*/
};
