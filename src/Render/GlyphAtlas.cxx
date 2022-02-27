#include "GlyphAtlas.hxx"
#include "../Util/Assert.hxx"
#include <algorithm>
#include <climits>
#include <iostream>
#include <math.h>
#include <string>
#include <sys/types.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include "freetype/ftimage.h"

static FT_Library library;

void InitFontRenderer() {
  FT_Error err = FT_Init_FreeType(&library);
  assume(err == FT_Err_Ok, "freetype failed to init");
}

/* TODO */
struct RectPacker {
  /* width and height of one slot */
  uint8_t slot_w, slot_h;
  /* dimensions of the bitmap in slots */
  uint8_t w, h;
  /* bitmap for slots*/
};

GlyphAtlas GenerateAtlas(std::string font_path, double pt_size) {
  /* TODO: switch to in memory font data instead of path */
  FT_Face face;
  FT_Error err;

  std::cerr << font_path << "\n";
  err = FT_New_Face(library, font_path.c_str(), 0, &face);
  /* TODO: handle gracefully */
  assume(err == FT_Err_Ok, "failed to load font face");

  err = FT_Set_Char_Size(face,
                         /* default char width */
                         0,
                         /* char_height in 1/64th of points */
                         (uint32_t)(pt_size * 64),
                         /* dpi TODO: look up dpi in registry */
                         96, 96);
  assume(err == FT_Err_Ok, "failed to set char size");

  GlyphAtlas atlas;
  atlas.glyphs.resize(UCHAR_MAX, {0, 0, 0, 0, 0, 0, 0});
  /* TODO: insert undefined char slot */

  size_t glyphs_present = 0;
  uint max_width = 0;
  uint max_height = 0;

  /* fill in all widths and heights of atlas */
  for (u_char i = 0; i < UCHAR_MAX; i++) {
    if (!isprint(i))
      /* TODO: assign to unknown char slot */
      continue;

    err = FT_Load_Char(face, i, FT_LOAD_RENDER | FT_LOAD_TARGET_LCD);
    if (err != FT_Err_Ok)
      continue;
    err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_LCD);
    if (err != FT_Err_Ok)
      continue;

    FT_GlyphSlot glyph = face->glyph;
    assume(glyph->bitmap.pixel_mode == FT_PIXEL_MODE_LCD,
           "incorrect pixel mode");
    /* TODO: preserve fractional part of 26.6 pixel format */
    atlas.glyphs[i].advance = glyph->advance.x >> 6;

    atlas.glyphs[i].w = glyph->bitmap.width / 3;
    atlas.glyphs[i].h = glyph->bitmap.rows;
    atlas.glyphs[i].padding_x = glyph->bitmap_left;
    atlas.glyphs[i].padding_y = glyph->bitmap_top;

    glyphs_present++;
    max_width = std::max(max_width, glyph->bitmap.width / 3);
    max_height = std::max(max_height, glyph->bitmap.rows);
  }

  /* length of texture by num of glyphs */
  size_t side_len = ceil(sqrt((float)glyphs_present));

  size_t glyph_row_bytes = side_len * max_width * 3;
  atlas.image.resize(glyph_row_bytes * max_height * side_len);

  /* TODO: fix */
  atlas.line_height = max_height * 1.5;
  atlas.image_w = side_len * max_width;
  atlas.image_h = side_len * max_height;

  /* position indexed by glyph */
  size_t x_pos = 0;
  size_t y_pos = 0;

  for (u_char i = 0; i < UCHAR_MAX; i++) {
    if (!isprint(i))
      /* TODO: assign to unknown char slot */
      continue;

    err = FT_Load_Char(face, i, FT_LOAD_RENDER | FT_LOAD_TARGET_LCD);
    if (err != FT_Err_Ok)
      continue;
    err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_LCD);
    if (err != FT_Err_Ok)
      continue;

    atlas.glyphs[i].x = x_pos * max_width;
    atlas.glyphs[i].y = y_pos * max_height;

    FT_Bitmap bitmap = face->glyph->bitmap;
    assume(bitmap.pixel_mode == FT_PIXEL_MODE_LCD, "incorrect pixel mode");
    uint8_t *src = bitmap.buffer;

    size_t row_len_bytes = max_width * side_len * 3;
    size_t row_start = y_pos * max_height * row_len_bytes;
    size_t col_offset = x_pos * max_width * 3;
    uint8_t *dst = atlas.image.data() + row_start + col_offset;

    for (size_t row = 0; row < bitmap.rows; row++) {
      memcpy(dst, src, bitmap.pitch);
      dst += row_len_bytes;
      src += bitmap.pitch;
    }

    x_pos++;
    if (x_pos == side_len) {
      x_pos = 0;
      y_pos++;
    }
  }

  /* TODO: release font data? or put in struct */
  return atlas;
}