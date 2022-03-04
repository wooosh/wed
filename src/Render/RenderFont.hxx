#pragma once

#include "BitMatrix2D.hxx"
#include "RenderContext.hxx"
#include "Types.hxx"
#include <optional>
#include <string_view>

#include <ft2build.h>
#include <unordered_map>
#include FT_FREETYPE_H
#include "freetype/ftimage.h"

#define GL_GLEXT_PROTOTYPES
#include "SDL_opengl.h"
#include "SDL_opengl_glext.h"

struct RenderFont {
  /* OpenType/TrueType allows a maximum of 65,536 glyphs in a font */
  using GlyphID = uint16_t;

  struct Glyph {
    Rect texture_region;
    vec2<int> offset;
    float advance;

    bool permanent;
  };

  RenderContext *rctx;

  FT_Face ft_face;
  float line_height;
  /* TODO: renderfont stores it's own indices, but puts vertices into the
   * rendercontext
   * TODO: renderfont.DrawFrame*/

  GPUTexture atlas;
  bool subpixel;
  size_t permanent_region_extent;
  std::vector<uint16_t> *indices;

  BitMatrix2D<BitMatrix2DFixedAxis::kHeight, uint32_t> used_space;
  BitMatrix2D<BitMatrix2DFixedAxis::kHeight, uint32_t> damaged_space;
  vec2<uint> tile_dimensions;

  std::unordered_map<GlyphID, Glyph> glyphs;

  /* TODO: prevent used glyphs from being evicted until the next frame,
   * just draw unknown char for now, but could store a boolean with each
   * glyph to tell if it was used since the previous frame draw, or a
   * counter telling the last frame it was used on, or a counter telling how
   * many frames ago it was last used. DO not resize texture if at all possible
   */

  /*
   * bool HasGlyph(uint16_t glyph_id);
   * uint16_t GetGlyphId(uint32_t codepoint);
   */
  Glyph GetGlyph(GlyphID, bool permanent);
  void DrawGlyph(RenderLayerIdx z, Point dst, GlyphID, Color color);
};

/* TODO: embed https://github.com/unicode-org/last-resort-font or hex codes */

/* THREAD-UNSAFE due to freetype, could be fixed with mutex */
std::optional<RenderFont> LoadFont(RenderContext *, std::string path,
                                   double pt_size);