#include "ViewEditor.hxx"
#include <cassert>
#include <chrono>
#include <iostream>

void ViewEditor::draw(Render::Backend &render) {
  draw_time = 0;
  const int digit_width = font.GlyphAdvance('0');
  assert(digit_width > 0);
  const uint gutter_width = (uint)digit_width * 5;

  auto t1 = std::chrono::high_resolution_clock::now();
  render.fillRect(viewport, Render::RGB(0xF7F4EF));
  render.fillRect({viewport.x, viewport.y, gutter_width, viewport.w},
                  Render::RGBA(0x00000022));
  auto t2 = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::micro> fill_time =
      std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
  draw_time += fill_time.count();

  size_t y = viewport.y;
  size_t line_num = first_line;
  TextBuffer::iterator i = buffer.AtLineCol(first_line, 0);

  std::string run;

  while (y + font.line_height < viewport.y + viewport.h && !i.IsEOF()) {
    /* draw gutter/line number */
    auto t1 = std::chrono::high_resolution_clock::now();
    render.drawText(viewport.y + digit_width, y, font,
                    std::to_string(line_num + 1).c_str(),
                    Render::RGB(0x999999));
    auto t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::micro> fill_time =
        std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
    draw_time += fill_time.count();

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
      int advance = font.GlyphAdvance(*i);
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

void ViewEditor::drawRun(Render::Backend &render, uint x, uint y,
                         const std::string &run) {
  /* TODO: drawText should not take a cstr */
  auto t1 = std::chrono::high_resolution_clock::now();
  if (!run.empty())
    render.drawText(x, y, font, run.c_str(), Render::RGB(0x111111));
  auto t2 = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double, std::micro> run_time =
      std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);

  draw_time += run_time.count();
}