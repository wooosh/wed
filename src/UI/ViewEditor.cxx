#include "ViewEditor.hxx"
#include "SDL_opengl_glext.h"
#include <cassert>
#include <chrono>
#include <iostream>

enum Layer { LayerBg, LayerGutter, LayerText };

/* TODO: subtract depth from 55, 0 is top layer */

void ViewEditor::draw(RenderContext &render) {
  draw_time = 0;
  const int digit_width = font.glyphs['0'].advance;
  assert(digit_width > 0);
  const uint gutter_width = (uint)digit_width * 5;

  render.DrawRect(LayerBg, viewport, RGB(0xf7f4ef));
  render.DrawRect(LayerGutter,
                  {viewport.x, viewport.y, gutter_width, viewport.h},
                  Dim(0.05));

  size_t y = viewport.y;
  size_t line_num = first_line;
  TextBuffer::iterator i = buffer.AtLineCol(first_line, 0);

  std::string run;

  while (y + font.line_height < viewport.y + viewport.h && !i.IsEOF()) {
    /* draw gutter/line number */
    drawRun(render, viewport.y + digit_width, y, std::to_string(line_num + 1));

    /* draw line */
    size_t width = 0;

    run.clear();
    while (true) {
      if (i.IsEOF()) {
        drawRun(render, viewport.x + gutter_width, y, run);
        break;
      }

      uint8_t c = *i;

      if (c == '\n') {
        /* commit run */
        drawRun(render, viewport.x + gutter_width, y, run);
        i++;
        y += font.line_height;
        line_num++;
        break;
      }

      if (isprint(c) == 0) {
        /* TODO: better unprintable char handling w/unicode */
        c = '?';
      }
      int advance = font.glyphs[*i].advance;
      if (width + advance >= viewport.w - gutter_width) {
        /* commit run */
        drawRun(render, viewport.x + gutter_width, y, run);
        y += font.line_height;
      } else {
        run.push_back(c);
        i++;
      }
    }
  }
}

void ViewEditor::drawRun(RenderContext &render, uint x, uint y,
                         const std::string &run) {
  (void)render;
  /* TODO: drawText should not take a cstr */
  auto t1 = std::chrono::high_resolution_clock::now();
  float pos = x;
  for (size_t i = 0; i < run.size(); i++) {
    if (isprint(run[i])) {
      font.DrawGlyph(LayerText, {(uint)pos, y + (uint)font.line_height}, run[i],
                     RGB(0x111111));
      pos += font.glyphs[run[i]].advance;
    }
  }

  auto t2 = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double, std::micro> run_time =
      std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);

  draw_time += run_time.count();
}