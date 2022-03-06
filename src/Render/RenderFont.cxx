#include "RenderFont.hxx"
#include "../Util/Assert.hxx"
#include "RenderContext.hxx"

#include <cctype>
#include <optional>
#include <string>

#include <ft2build.h>
#include FT_FREETYPE_H
#include "freetype/ftimage.h"

static FT_Library library = nullptr;

/* WARNING: due to the way freetype works, calling this function will invalidate
 * any previously returned FT_bitmaps */
static std::optional<FT_Bitmap>
RenderFreeTypeGlyph(FT_Face face, RenderFont::GlyphID g, bool subpixel) {

  int32_t flags = FT_LOAD_RENDER;
  if (subpixel) {
    flags |= FT_LOAD_TARGET_LCD;
  } else {
    /* TODO: investigate whether or not this actually disables subpixel
     * rendering entirely */
    flags |= FT_LOAD_TARGET_NORMAL;
  }

  FT_Error err = FT_Load_Char(face, g, flags);
  expect(err == FT_Err_Ok);

  FT_Bitmap bitmap = face->glyph->bitmap;
  if (subpixel) {
    /* TODO: should we round up? */
    bitmap.width = bitmap.width / 3u;
  }

  return bitmap;
}

std::optional<RenderFont> LoadFont(RenderContext *rctx, std::string path,
                                   double pt_size) {
  if (library == nullptr) {
    FT_Error err = FT_Init_FreeType(&library);
    assume(err == FT_Err_Ok, "freetype failed to init");
  }

  FT_Error err;
  RenderFont rf;
  rf.subpixel = true;
  rf.rctx = rctx;

  err = FT_New_Face(library, path.c_str(), 0, &rf.ft_face);
  expect(err == FT_Err_Ok);

  err = FT_Set_Char_Size(rf.ft_face,
                         /* default char width */
                         0,
                         /* char_height in 1/64th of points */
                         (uint32_t)(pt_size * 64),
                         /* dpi TODO: look up dpi in registry */
                         96, 96);

  /* freetype uses 26.6 fixed point for line height values */
  rf.line_height = (float)rf.ft_face->size->metrics.height / (float)(1 << 6);

  /* determine slot size */
  auto em_bitmap = RenderFreeTypeGlyph(rf.ft_face, 'M', rf.subpixel);
  expect(em_bitmap);

  constexpr uint x_sections = 6;
  constexpr uint y_sections = 3;
  rf.tile_dimensions = {DivideRoundUp(em_bitmap->width, x_sections),
                        DivideRoundUp(em_bitmap->rows, y_sections)};

  /* size texture and bitmatrices to accomodate 256 M
   * sized characters */
  constexpr size_t target_num_glyphs = 256;

  size_t sections;
  if constexpr (rf.used_space.axis == BitMatrix2DFixedAxis::kWidth) {
    sections = x_sections;
  } else {
    sections = y_sections;
  }

  size_t num_glyphs_per_row = rf.used_space.major_len / sections;
  size_t rows_needed = DivideRoundUp(target_num_glyphs, num_glyphs_per_row);

  rf.used_space.Resize(rows_needed * 2);
  rf.damaged_space.Resize(rows_needed * 2);

  GPUTexture::Format format;
  if (rf.subpixel) {
    format = GPUTexture::Format::kRGB;
  } else {
    format = GPUTexture::Format::kGrayscale;
  }

  rf.atlas = RenderContext::CreateTexture(
      format, rf.used_space.width() * rf.tile_dimensions.x,
      rf.used_space.height() * rf.tile_dimensions.y, nullptr);

  /* load permanently mapped characters (ASCII printable
   * 0-127) */
  for (char c = 0; c < 127; c++) {
    if (std::isprint(c)) {
      Rect region = rf.GetGlyph(c, true).texture_region;

      size_t extent;
      if constexpr (rf.used_space.axis == BitMatrix2DFixedAxis::kWidth) {
        extent = region.x + region.w;
      } else {
        extent = region.y + region.h;
      }

      rf.permanent_region_extent = std::max(rf.permanent_region_extent, extent);
    }
  }

  rf.batch = rctx->NewBatch({rf.atlas, true, {}});

  return rf;
}

RenderFont::Glyph RenderFont::GetGlyph(GlyphID glyph_id, bool permanent) {
  /* TODO: use permanent_region_extent and handle overflows */
  auto glyph_idx = glyphs.find(glyph_id);
  if (glyph_idx != glyphs.end()) {
    return glyph_idx->second;
  }

  auto bitmap = RenderFreeTypeGlyph(ft_face, glyph_id, subpixel);
  /* TODO: handle this without panicing */
  assume(bitmap, "freetype could not provide glyph");

  size_t tile_w = DivideRoundUp(bitmap->width, tile_dimensions.x);
  size_t tile_h = DivideRoundUp(bitmap->rows, tile_dimensions.y);
  auto pos = used_space.FindUnsetRect(tile_w, tile_h);
  /* TODO: handle this without panicing */
  assume(pos, "could not find space in bitmatrix");
  used_space.SetRect({(int)pos->x, (int)pos->y, (int)tile_w, (int)tile_h});

  /* freetype uses 26.6 fixed point for advance values */
  float advance = (float)ft_face->glyph->advance.x / (float)(1 << 6);

  Glyph glyph = {{(int)pos->x * (int)tile_dimensions.x,
                  (int)pos->y * (int)tile_dimensions.y, (int)bitmap->width,
                  (int)bitmap->rows},
                 {ft_face->glyph->bitmap_left, ft_face->glyph->bitmap_top},
                 advance,
                 permanent};

  glyphs.insert({glyph_id, glyph});

  /* copy the freetype bitmap into our own buffer, since the pitch of it's
   * bitmap does not correspond with the width, and can also be negative, which
   * opengl has difficulty with */
  std::vector<uint8_t> bitmap_copy(bitmap->width * 3 * 3 * bitmap->rows);
  uint8_t *src = bitmap->buffer;
  uint8_t *dst = bitmap_copy.data();
  for (size_t i = 0; i < bitmap->rows; i++) {
    memcpy(dst, src, bitmap->pitch);
    src += bitmap->pitch;
    dst += bitmap->width * 3;
  }

  RenderContext::CopyIntoTexture(atlas, glyph.texture_region,
                                 bitmap_copy.data());

  return glyph;
}

void RenderFont::DrawGlyph(RenderLayerIdx z, Point dst, GlyphID glyph_id,
                           Color color) {
  Glyph glyph = GetGlyph(glyph_id, false);

  rctx->PushQuad(batch, z, {dst.x + glyph.offset.x, dst.y - glyph.offset.y},
                 glyph.texture_region.top_left(), glyph.texture_region.w,
                 glyph.texture_region.h, color);
}