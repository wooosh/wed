#include "Backend.hxx"
#include "../Util/Assert.hxx"
#include "SDL.h"
#include "SDL_ttf.h"
#include <cassert>
#include <optional>

static SDL_Rect toSDLRect(Render::Rect r) {
  SDL_Rect sdl_rect;
  sdl_rect.x = r.x;
  sdl_rect.y = r.y;
  sdl_rect.w = r.w;
  sdl_rect.h = r.h;
  return sdl_rect;
}

/*
static Render::Rect fromSDLRect(SDL_Rect r) {
  Render::Rect rect;
  rect.x = r.x;
  rect.y = r.y;
  rect.w = r.w;
  rect.h = r.h;
  return rect;
}*/

static SDL_Color toSDLColor(Render::Color c) {
  SDL_Color sdl_color;
  sdl_color.r = c.r;
  sdl_color.g = c.g;
  sdl_color.b = c.b;
  sdl_color.a = c.a;
  return sdl_color;
}

namespace Render {
Backend::Backend(SDL_Window *win) {
  assert(win != nullptr);
  window = win;

  bool success = SDL_SetHint(SDL_HINT_RENDER_BATCHING, "1");
  assume(success, "render batching required");
  sdl_render = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
  assume(sdl_render != nullptr, SDL_GetError);

  int err = SDL_SetRenderDrawBlendMode(sdl_render, SDL_BLENDMODE_BLEND);
  assume(err == 0, SDL_GetError);
}

Backend::~Backend() { SDL_DestroyRenderer(sdl_render); }

void Backend::clear() {
  int err = SDL_SetRenderDrawColor(sdl_render, 255, 255, 255, SDL_ALPHA_OPAQUE);
  assume(err == 0, SDL_GetError);

  err = SDL_RenderClear(sdl_render);
  assume(err == 0, SDL_GetError);
}

// TODO: sdl_drawRects for batching?
void Backend::fillRect(Rect rect, Color color) {
  int err =
      SDL_SetRenderDrawColor(sdl_render, color.r, color.g, color.b, color.a);
  assume(err == 0, SDL_GetError);

  SDL_Rect sdl_rect = toSDLRect(rect);
  err = SDL_RenderFillRect(sdl_render, &sdl_rect);
  assume(err == 0, SDL_GetError);
}

void Backend::lineRect(Rect rect, Color color) {
  int err =
      SDL_SetRenderDrawColor(sdl_render, color.r, color.g, color.b, color.a);
  assume(err == 0, SDL_GetError);

  SDL_Rect sdl_rect = toSDLRect(rect);
  err = SDL_RenderDrawRect(sdl_render, &sdl_rect);
  assume(err == 0, SDL_GetError);
}

void Backend::drawText(uint x, uint y, Font &font, const char *text,
                       Color color) {
  SDL_Surface *surface =
      TTF_RenderUTF8_Blended(font.sdl_font, text, toSDLColor(color));
  assume(surface != nullptr, TTF_GetError);

  auto texture = SDL_CreateTextureFromSurface(sdl_render, surface);
  assume(texture != nullptr, SDL_GetError);
  SDL_FreeSurface(surface);

  SDL_Rect dst_rect = {.x = (int)x, .y = (int)y, .w = 0, .h = 0};
  int err = SDL_QueryTexture(texture, NULL, NULL, &dst_rect.w, &dst_rect.h);
  assume(err == 0, SDL_GetError);

  err = SDL_RenderCopy(sdl_render, texture, NULL, &dst_rect);
  assume(err == 0, SDL_GetError);

  SDL_DestroyTexture(texture);
}

void Backend::commit() { SDL_RenderPresent(sdl_render); }
} // namespace Render