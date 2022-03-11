#include "ViewEditor.hxx"
#include "SDL_opengl_glext.h"
#include <cassert>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <sys/types.h>

enum Layer { LayerBg, LayerGutter, LayerText, LayerCursor };

void ViewEditor::ScrollPx(int amount) {
  if (amount == 0)
    return;

  offset_px += amount;
  ssize_t line_delta = offset_px / (int64_t)font.line_height;
  if (offset_px < 0) {
    // we only need to fix the last line
    line_delta--;
    offset_px = font.line_height + (offset_px % (int64_t)font.line_height);
    if (-line_delta > (ssize_t)first_line) {
      offset_px = 0;
    }
    line_delta += offset_px / (int64_t)font.line_height;
  }

  line_delta = std::max(-(ssize_t)first_line, line_delta);
  line_delta =
      std::min((ssize_t)(buffer.num_lines - first_line) - 1, line_delta);

  first_line += line_delta;
  if (first_line == buffer.num_lines - 1) {
    offset_px = std::min(offset_px, (int64_t)font.line_height);
  } else {
    offset_px %= (int64_t)font.line_height;
  }
  /*
  printf("%d == %zd == %zd*%zd + %zd", amount,
         line_delta * (int64_t)font.line_height + offset_px - ooo, line_delta,
         (int64_t)font.line_height, offset_px - old_offset_px);*/
}

void ViewEditor::ScrollLines(int amount) {
  target_px += amount * font.line_height;
}

void ViewEditor::PageUp(void) { ScrollLines(-viewport.h / font.line_height); }

void ViewEditor::PageDown(void) { ScrollLines(viewport.h / font.line_height); }

void ViewEditor::draw(RenderContext &render) {
  /* apply scroll velocity TODO: frame update vs draw */
  constexpr double anim_factor = 3;
  int64_t move_amount = (target_px - progress_target) / anim_factor;
  if (std::abs(target_px - progress_target) < anim_factor) {
    move_amount = target_px - progress_target;
    progress_target = 0;
    target_px = 0;
  } else {
    progress_target += move_amount;
  }
  ScrollPx(move_amount);

  is_animating = target_px != 0;

  Hash inputs = Hasher()
                    .add(first_line)
                    .add(offset_px)
                    .add(buffer.epoch)
                    .add(viewport.w)
                    .add(viewport.h)
                    .add(viewport.x)
                    .add(viewport.y)
                    .add(cursor->span_idx)
                    .add(cursor->byte_offset);
  if (inputs == last_inputs)
    return;
  last_inputs = inputs;

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
    float x = 0;

    run.clear();
    while (y < viewport.y + viewport.h) {
      if (i == *cursor) {
        /* TODO: use font metrics to figure out positioning */
        render.DrawRect(LayerCursor,
                        {viewport.x + gutter_width + padding + (int)x, y + 4, 2,
                         (int)font.line_height},
                        RGB(0x555555));
      }
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
        x = 0;
        break;
      }

      if (isprint(c) == 0) {
        /* TODO: better unprintable char handling w/unicode */
        c = '?';
      }
      int advance = font.glyphs[*i].advance;
      if (x + advance >= viewport.w - gutter_width) {
        /* commit run */
        drawRun(render, viewport.x + gutter_width + padding, y, run);
        run.clear();
        y += font.line_height;
        x = 0;
      } else {
        run.push_back(c);
        // TODO: won't work if the glyph isn't already in the atlas
        x += font.glyphs[c].advance;
        i++;
      }
    }
  }
}

void ViewEditor::drawRun(RenderContext &render, int x, int y,
                         const std::string &run) {
  (void)render;
  float pos = x;
  for (size_t i = 0; i < run.size(); i++) {
    if (isprint(run[i])) {
      font.DrawGlyph(LayerText, {(int)pos, y + (int)font.line_height}, run[i],
                     RGB(0x111111));
      pos += font.glyphs[run[i]].advance;
    }
  }
}