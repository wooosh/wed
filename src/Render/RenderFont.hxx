#pragma once

#include "Types.hxx"
#include <optional>
#include <string_view>

#include <ft2build.h>
#include FT_FREETYPE_H
#include "freetype/ftimage.h"

#define GL_GLEXT_PROTOTYPES
#include "SDL_opengl.h"
#include "SDL_opengl_glext.h"

struct RenderFont {
  struct Glyph {
    Rect texture_area;
    vec2<int> offset;
    bool permanent;
  };

  FT_Face ft_face;

  GLuint texture_id;
  vec2<uint> texture_dimensions;

  Bitset2D free_space;
  Bitset2D damaged_space;
  vec2<uint> tile_dimensions;
  vec2<uint> bitset_dimensions;

  /* TODO: prevent used glyphs from being evicted until the next frame,
   * just draw unknown char for now, but could store a boolean with each
   * glyph to tell if it was used since the previous frame draw, or a
   * counter telling the last frame it was used on, or a counter telling how
   * many frames ago it was last used */

  /*
   * bool HasGlyph(uint16_t glyph_id);
   * uint16_t GetGlyphId(uint32_t codepoint);
   */
  Glyph GetGlyph(uint16_t glyph_id);
};

std::optional<RenderFont> LoadFont(std::string_view path, double pt_size);