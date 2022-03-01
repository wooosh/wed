#include "GlyphAtlas.hxx"
#include "../Util/Assert.hxx"
#include "TileMap.hxx"
#include <algorithm>
#include <chrono>
#include <climits>
#include <iostream>
#include <math.h>
#include <string>
#include <sys/types.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include "freetype/ftimage.h"

static FT_Library library;

/* TODO: handle bitmaps */
/* TODO:
 * - keep track of lowest extent during ASCII tilemap process
 * - create new tilemap rounded up to to multiple of tile height
 * - when overflow, create new epoch, clear damaged bitmap
 * - use ring buffer of glyphs, keep track of glyphs epoch and rect
 * - when using a non ascii glyph, check the epoch
 * - check for runs of ascii to skip all of this
 */

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
    // if (glyph->bitmap.width <= 0 || glyph->bitmap.rows <= 0) {
    //  continue;
    //}
    /* TODO: preserve fractional part of 26.6 pixel format */
    atlas.glyphs[i].advance = (glyph->advance.x >> 6) + 1;

    atlas.glyphs[i].w = glyph->bitmap.width / 3;
    atlas.glyphs[i].h = glyph->bitmap.rows;
    atlas.glyphs[i].padding_x = glyph->bitmap_left;
    atlas.glyphs[i].padding_y = glyph->bitmap_top;

    glyphs_present++;
    max_width = std::max(max_width, glyph->bitmap.width / 3);
    max_height = std::max(max_height, glyph->bitmap.rows);
  }

  /* partition texture into 3 parts: ascii/unevictable, unicode pt1, unicode pt2
   * keep track of lowest extent during building process
   */

  /* TODO: ascii tilemap for start layout, and separate unicode tilemap */
  TileMap tilemap((atlas.glyphs['M'].w / 3) + 1, (atlas.glyphs['T'].h / 3) + 1,
                  64);

  atlas.image_w = tilemap.tile_w * TILEMAP_ROW_BITS;
  atlas.image_h = tilemap.tile_h * tilemap.bitmap.size();

  atlas.image.resize(atlas.image_w * atlas.image_h * 3);

  /* used for debugging */
  size_t atlas_pitch = atlas.image_w * 3;
  for (size_t i = 0; i < atlas.image.size(); i += 3) {
    atlas.image[i + 2] = 0xff;
  }

  /* TODO: fix */
  atlas.line_height = max_height * 1.5;
  std::chrono::duration<double, std::micro> rectpack_time =
      std::chrono::hours(0);
  size_t packed = 0;
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

    Glyph &glyph = atlas.glyphs[i];
    if (glyph.w <= 0 || glyph.h <= 0) {
      continue;
    }
    packed++;
    auto t1 = std::chrono::high_resolution_clock::now();
    Point texture_pos = tilemap.AllocateRect(glyph.w, glyph.h);
    auto t2 = std::chrono::high_resolution_clock::now();
    rectpack_time +=
        std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);
    glyph.x = texture_pos.x;
    glyph.y = texture_pos.y;

    /* copy bitmap */
    FT_Bitmap bitmap = face->glyph->bitmap;
    assume(bitmap.pixel_mode == FT_PIXEL_MODE_LCD, "incorrect pixel mode");
    uint8_t *src = bitmap.buffer;

    uint8_t *dst =
        atlas.image.data() + atlas_pitch * texture_pos.y + 3 * texture_pos.x;

    for (size_t row = 0; row < bitmap.rows; row++) {
      memcpy(dst, src, bitmap.pitch);
      dst += atlas_pitch;
      src += bitmap.pitch;
    }
  }

  /*
  for (size_t i = 0; i < tilemap.bitmap.size(); i++) {
    for (size_t x = 0; x < atlas.image_w * 3; x += 3) {
      atlas.image[(i * atlas_pitch * tilemap.tile_h) + x] = 0xcc;
    }
  }

  for (size_t i = 0; i < TILEMAP_ROW_BITS; i++) {
    for (size_t x = 0; x < atlas.image_h; x++) {
      atlas.image[(x * atlas_pitch) + i * tilemap.tile_w * 3 + 1] = 0xcc;
    }
  }*/

  printf("avg rectpack time %fns\n",
         (double)rectpack_time.count() / (double)packed);
  printf("total rectpack time %fns\n", (double)rectpack_time.count());

  /* TODO: release font data? or put in struct */
  return atlas;
}