#include "Platform/LocateFont.hxx"
#include "Render/RenderContext.hxx"
#include "SDL.h"
#include "SDL_error.h"
#include "SDL_events.h"
#include "SDL_keycode.h"
#include "SDL_timer.h"
#include "SDL_video.h"
#include "TextBuffer/TextBuffer.hxx"
#include "UI/View.hxx"
#include "UI/ViewEditor.hxx"
#include "Util/Assert.hxx"
#include "src/Render/RenderFont.hxx"
#include <chrono>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>

#define GL_GLEXT_PROTOTYPES
#include "SDL_opengl.h"

/* TODO:
 * - fix rendercontext to use indices properly
 */

void readFile(TextBuffer &tb, const char *filename) {
  std::ifstream file(filename, std::ios::binary);
  file.unsetf(std::ios::skipws);

  file.seekg(0, std::ios::end);
  std::streampos file_size;
  file_size = file.tellg();
  file.seekg(0, std::ios::beg);

  /* TODO: this is stupid, we can do this without copying everything into a
   * vector with something like TextBuffer::InsertAtN */
  std::vector<char> data;
  data.reserve(file_size);
  data.insert(data.begin(), std::istreambuf_iterator<char>(file),
              std::istreambuf_iterator<char>());

  tb.InsertAt(tb.AtByteOffset(0), data.begin(), data.end());
}

int main(int argc, char **argv) {
  assume(argc == 3,
         "expects exactly one font argument and one filename argument");

  // LocateFont init
  assume(LocateFontInit(), "failed to init font locator");
  std::optional<std::string> font_path = LocateFontFile(
      {argv[1], 24.0, FontFaceProperties::WEIGHT_REGULAR,
       FontFaceProperties::STRETCH_MEDIUM, FontFaceProperties::SLANT_NORMAL});
  assume(font_path.has_value(), "couldn't locate font");

  // SDL2 init
  int err = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
  assume(err == 0, SDL_GetError);
  // allow the screen to blank
  SDL_EnableScreenSaver();
  SDL_EventState(SDL_FINGERMOTION, SDL_ENABLE);
  atexit(SDL_Quit);

  // create window
  // get screen size
  SDL_DisplayMode dm;
  err = SDL_GetCurrentDisplayMode(0, &dm);
  assume(err == 0, SDL_GetError);

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  SDL_Window *window = SDL_CreateWindow(
      "sdl", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, dm.w * 0.8,
      dm.h * 0.8, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
  assume(window != nullptr, SDL_GetError);

  SDL_GLContext context = SDL_GL_CreateContext(window);
  SDL_GL_SetSwapInterval(0);
  RenderContext rctx(window);
  int w, h;
  SDL_GetWindowSize(window, &w, &h);
  rctx.Init();
  auto font = LoadFont(&rctx, font_path.value(), 12.0);
  assume(font, "could not render font");

  TextBuffer tb;
  readFile(tb, argv[2]);

  auto editor = ViewEditor(*font, tb);
  editor.first_line = 0;
  editor.cursor = tb.PersistIterator(tb.AtByteOffset(0));
  auto root = ViewRoot(editor);

  SDL_Event event;
  bool running = true;
  const uint64_t frame_ms = 1000 / 60;
  uint64_t last_render_time = 0;
  int frame_num = 0;

  float scroll_accum = 0;

  while (running) {
    /* TODO: error handling */
    int event_present = 0;
    int64_t remaining_time = frame_ms - (SDL_GetTicks64() - last_render_time);
    if (remaining_time > 2) {
      event_present = SDL_WaitEventTimeout(&event, remaining_time);
    }

    if (event_present) {
      switch (event.type) {
      case SDL_MOUSEWHEEL:
        scroll_accum += event.wheel.preciseY;
        break;

      case SDL_TEXTINPUT: {
        std::string_view text(event.text.text);
        tb.InsertAt(*editor.cursor, text.begin(), text.end());
        break;
      }

      case SDL_KEYDOWN:
        switch (event.key.keysym.sym) {
        case SDLK_UP:
          if (event.key.keysym.mod & KMOD_SHIFT) {
            editor.PageUp();
          } else {
            editor.ScrollLines(-1);
          }
          break;
        case SDLK_DOWN:
          if (event.key.keysym.mod & KMOD_SHIFT) {
            editor.PageDown();
          } else {
            editor.ScrollLines(1);
          }
          break;
        case SDLK_RIGHT:
          if (*editor.cursor < editor.buffer.end())
            (*editor.cursor)++;
          break;
        case SDLK_LEFT:
          if (editor.buffer.begin() < *editor.cursor)
            (*editor.cursor)--;
          break;
        case SDLK_RETURN:
          std::string_view newline = "\n";
          tb.InsertAt(*editor.cursor, newline.begin(), newline.end());
          break;
        }
        break;
      case SDL_MOUSEBUTTONDOWN:
      case SDL_QUIT:
        running = false;
        break;
      }
    }

    if (SDL_GetTicks64() - last_render_time > frame_ms) {
      editor.target_px += -scroll_accum * 3 * (int)font->line_height;
      scroll_accum = 0;
      auto t1 = std::chrono::high_resolution_clock::now();
      root.draw(rctx);
      auto t2 = std::chrono::high_resolution_clock::now();
      rctx.Commit();
      auto t3 = std::chrono::high_resolution_clock::now();

      std::chrono::duration<double, std::micro> layout_time =
          std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
      std::chrono::duration<double, std::micro> commit_time =
          std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2);

      if (frame_num % 60 == 0) {
        std::cerr << "\n1 second average\n";
        std::cerr << "layout: " << layout_time.count() << "us\n";
        std::cerr << "commit: " << commit_time.count() << "us\n";
        std::cerr << "total:  " << layout_time.count() + commit_time.count()
                  << "us\n";
        std::cerr << "fps: "
                  << 1000000 / (layout_time.count() + commit_time.count())
                  << " fps\n";
      }
      last_render_time = SDL_GetTicks64();
      frame_num++;
      frame_num %= 60;
    }
  }

  LocateFontDeinit();
  SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(window);
  SDL_Quit();
}
