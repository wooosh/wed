#include "ViewEditor.hxx"
#include "SDL_opengl_glext.h"
#include <cassert>
#include <chrono>
#include <iostream>
#include <sys/types.h>

enum Layer { LayerBg, LayerGutter, LayerText };

void ViewEditor::ScrollPx(int amount) {
  offset_px += amount;
  /*
  if (offset_px > (int)font.line_height && first_line == buffer.num_lines - 1) {
    offset_px = font.line_height - 1;
  }*/
  ssize_t line_delta = offset_px / (int64_t)font.line_height;

  if (offset_px < 0) {
    line_delta--;
    offset_px = font.line_height + offset_px;
    if (-line_delta > (ssize_t)first_line) {
      offset_px = 0;
    }
  }

  line_delta = std::max(-(ssize_t)first_line, line_delta);
  line_delta =
      std::min((ssize_t)(buffer.num_lines - first_line) - 1, line_delta);

  first_line += line_delta;
  if (first_line == buffer.num_lines - 1) {
    offset_px = std::min(offset_px, (int64_t)font.line_height);
  } else {
    offset_px %= (int)font.line_height;
  }
}

void ViewEditor::draw(RenderContext &render) {
  /* apply scroll velocity TODO: frame update vs draw */
  constexpr double anim_factor = 3;
  ScrollPx(remaining_delta / anim_factor);
  remaining_delta -= remaining_delta / anim_factor;
  if (std::abs(remaining_delta) < 1) {
    remaining_delta = 0;
  }

  const int digit_width = font.glyphs['0'].advance;
  assert(digit_width > 0);
  const int gutter_width = (uint)digit_width * 5;
  const int padding = 3;

  render.DrawRect(LayerBg, viewport, RGB(0xf7f4ef));
  render.DrawRect(LayerGutter,
                  {viewport.x, viewport.y, gutter_width, viewport.h}, Dim(0.1));

  int y = viewport.y - offset_px;
  size_t line_num = first_line;
  TextBuffer::iterator i = buffer.AtLineCol(first_line, 0);

  std::string run;

  while (y < viewport.y + viewport.h && !i.IsEOF()) {
    /* draw gutter/line number */
    drawRun(render, viewport.x + digit_width, y, std::to_string(line_num + 1));

    /* draw line */
    int width = 0;

    run.clear();
    while (true) {
      if (i.IsEOF()) {
        drawRun(render, viewport.x + gutter_width + padding, y, run);
        break;
      }

      uint8_t c = *i;

      if (c == '\n') {
        /* commit run */
        drawRun(render, viewport.x + gutter_width + padding, y, run);
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
        drawRun(render, viewport.x + gutter_width + padding, y, run);
        y += font.line_height;
      } else {
        run.push_back(c);
        i++;
      }
    }
  }
}

void ViewEditor::drawRun(RenderContext &render, int x, int y,
                         const std::string &run) {
  (void)render;
  /* TODO: drawText should not take a cstr */
  float pos = x;
  for (size_t i = 0; i < run.size(); i++) {
    if (isprint(run[i])) {
      font.DrawGlyph(LayerText, {(int)pos, y + (int)font.line_height}, run[i],
                     RGB(0x111111));
      pos += font.glyphs[run[i]].advance;
    }
  }
}