#include "TextBuffer/TextBuffer.hxx"
#include "UI/Backend.hxx"
#include "UI/Render.hxx"
#include "UI/View.hxx"
#include "UI/ViewEditor.hxx"
#include "Util/Assert.hxx"
#include <SDL2/SDL.h>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>

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
  assume(argc == 2, "expects exactly one filename argument");
  TextBuffer tb;
  readFile(tb, argv[1]);

  // SDL2 init
  int err = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
  assume(err == 0, SDL_GetError);
  // allow the screen to blank
  SDL_EnableScreenSaver();
  atexit(SDL_Quit);

  // SDL2_ttf init
  err = TTF_Init();
  assume(err == 0, TTF_GetError);
  atexit(TTF_Quit);

  // create window
  // get screen size
  SDL_DisplayMode dm;
  err = SDL_GetCurrentDisplayMode(0, &dm);
  assume(err == 0, SDL_GetError);

  SDL_Window *window =
      SDL_CreateWindow("sdl", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                       dm.w * 0.8, dm.h * 0.8, SDL_WINDOW_RESIZABLE);
  exists(window, SDL_GetError);
  auto font = Render::Font("/usr/share/fonts/TTF/DejaVuSansMono.ttf", 14);
  auto backend = Render::Backend(window);

  auto editor = ViewEditor(font, tb);
  editor.first_line = 0;
  auto root = ViewRoot(editor);

  for (size_t i = 0; i < 2 * 60; i++) {
    SDL_Delay(1000 / 60);
    editor.first_line++;
    backend.clear();
    auto t1 = std::chrono::high_resolution_clock::now();
    root.draw(backend);
    auto t2 = std::chrono::high_resolution_clock::now();
    backend.commit();
    auto t3 = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::micro> layout_time =
        std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
    std::chrono::duration<double, std::micro> commit_time =
        std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2);

    std::cerr << "draw:   " << editor.draw_time << "us\n";
    std::cerr << "layout: " << layout_time.count() << "us\n";
    std::cerr << "commit: " << commit_time.count() << "us\n";
    std::cerr << "total:  " << layout_time.count() + commit_time.count()
              << "us\n";
    std::cerr << "nodraw: " << layout_time.count() - editor.draw_time << "us\n";
  }

  // SDL_Delay(1000);

  SDL_DestroyWindow(window);
  SDL_Quit();
}
