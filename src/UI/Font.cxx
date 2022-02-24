#include "../Util/Assert.hxx"
#include "Render.hxx"
#include <SDL_ttf.h>
#include <cstdint>
#include <iostream>
#include <string>

namespace Render {
Font::Font(const char *filename, int pt_size) {
  sdl_font = TTF_OpenFont(filename, pt_size);
  exists(sdl_font, TTF_GetError);
  // TODO: null check + TTF_GetError
  line_height = TTF_FontLineSkip(sdl_font);
  TTF_SetFontHinting(sdl_font, TTF_HINTING_LIGHT);
}

Font::~Font() { TTF_CloseFont(sdl_font); }

int Font::GlyphAdvance(uint32_t glyph) {
  // TODO: handle u32 codepoints properly, or atleast don't make them exit
  assume(glyph <= UINT16_MAX, "SDL2_TTF only supports u16 codepoints");
  int advance;
  /* TODO: change to TTF_GlyphMetric32, might require a more recent version of
   * SDL2_TTF */
  int err = TTF_GlyphMetrics(sdl_font, (uint16_t)glyph, nullptr, nullptr,
                             nullptr, nullptr, &advance);
  assume(err == 0, TTF_GetError);
  return advance;
}

void Font::textDimensions(const std::string &str, uint &w, uint &h) {
  int err = TTF_SizeUTF8(sdl_font, str.c_str(), (int *)&w, (int *)&h);
  assume(err == 0, TTF_GetError);
}

void Font::maxCharacters(const std::string &str, uint max_width,
                         uint &num_chars, uint &actual_width) {
  (void)str;
  (void)max_width;
  (void)num_chars;
  (void)actual_width;
  // TTF_MeasureUTF8();
  assume(0, "not supported");
}

} // namespace Render