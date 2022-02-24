#pragma once

#include "Render.hxx"
#include <SDL2/SDL.h>

namespace Render {
struct Backend {
  SDL_Window *window;
  SDL_Renderer *sdl_render;
  Rect viewport;

  Backend(SDL_Window *);
  ~Backend();

  Backend(const Backend &) = delete;
  Backend &operator=(const Backend &) = delete;

  /* TODO: clipping */

  // drawing methods
  // blank screen to black
  void clear();

  void lineRect(Rect, Color);
  void fillRect(Rect, Color);

  void drawImage(uint x, uint y, Image, Rect image_region);
  void drawText(uint x, uint y, Font &, const char *text, Color);

  // apply any pending changes to the screen
  void commit();
};
} // namespace Render