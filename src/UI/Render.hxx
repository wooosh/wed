#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "SDL_ttf.h"

namespace Render {
struct Color {
  uint8_t b, g, r, a;
};

constexpr Color RGBA(uint32_t lit) {
  return {.b = (uint8_t)(lit >> 8),
          .g = (uint8_t)(lit >> 16),
          .r = (uint8_t)(lit >> 24),
          .a = (uint8_t)(lit)};
}

constexpr Color RGB(uint32_t lit) {
  return {.b = (uint8_t)(lit),
          .g = (uint8_t)(lit >> 8),
          .r = (uint8_t)(lit >> 16),
          .a = 255};
}

struct Rect {
  uint x, y, w, h;
};

struct Image {
  std::vector<Color> pixels;
  uint width, height;
};

struct Font {
  TTF_Font *sdl_font;
  uint line_height;

  Font(const char *file, int pt_size);
  ~Font();

  Font(const Font &) = delete;
  Font &operator=(const Font &) = delete;

  int GlyphAdvance(uint32_t glyph);

  // calculates the width and height of the text if it was drawn
  // undefined behavior if the string contains glyphs that cannot be drawn (such
  // as null), it results in undefined behavior
  void textDimensions(const std::string &, uint &w, uint &h);

  // calculates the number of characters that fit in max_width px, as well as
  // the actual width. if the string contains glyphs that cannot be drawn (such
  // as null), it results in undefined behavior
  void maxCharacters(const std::string &, uint max_width, uint &num_chars,
                     uint &actual_width);
};
} // namespace Render