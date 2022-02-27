#pragma once

#include <cstdint>
#include <string>
#include <sys/types.h>
#include <vector>

struct Glyph {
  float advance;
  int padding_x, padding_y;
  /* atlas coordinates (pixels) */
  uint x, y, w, h;
};

struct GlyphAtlas {
  size_t line_height;
  /* prefilled to UCHAR_MAX */
  std::vector<Glyph> glyphs;

  /* pixels*/
  uint image_w, image_h;
  /* RGB format */
  std::vector<uint8_t> image;
};

void InitFontRenderer();
GlyphAtlas GenerateAtlas(std::string font_path, double pt_size);
