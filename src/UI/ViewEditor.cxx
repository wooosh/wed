#include "ViewEditor.hxx"
#include "SDL_opengl_glext.h"
#include "src/Render/Types.hxx"
#include <cassert>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <sys/types.h>

enum Layer { LayerBg, LayerGutter, LayerText, LayerCursor };

void ViewEditor::ScrollPx(int amount) {
  if (amount == 0)
    return;

  offset_px += amount;

  if (first_line == 0 && offset_px < 0)
    offset_px = 0;

  if (1)
    return;

  /* TODO: account for line height not being constant due to wrapping */
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

int NumDigits(size_t num) {
  int len = 1;
  while (num /= 10)
    len++;
  return len;
}

int ViewEditor::CalculateGutterWidth(void) {
  const int digit_width = font.glyphs['0'].advance;
  assert(digit_width > 0);
  const int line_no_digits = 2 + NumDigits(buffer.num_lines);
  /* 2 digit padding */
  return digit_width * (line_no_digits + 2);
}

void ViewEditor::NormalizeCursor(void) {
  const int textarea_w = viewport.w - CalculateGutterWidth();

  float x = 0;
  uint16_t current_line_height = font.line_height;

  TextBuffer::iterator iter = buffer.AtLineCol(first_line, 0);

  if (first_line == 0 && offset_px < 0) {
    offset_px = 0;
    return;
  }

  if (offset_px > 0) {
    while (!iter.IsEOF()) {
      uint8_t c = *iter;
      iter++;

      if (iter.IsEOF()) {
        offset_px = std::min(offset_px, (int64_t)current_line_height);
        break;
      }

      if (c == '\n') {
        if (offset_px > current_line_height) {
          first_line++;
          offset_px -= current_line_height;
        } else {
          break;
        }
        x = 0;
        current_line_height = font.line_height;
        continue;
      }

      if (!isprint(c)) {
        c = '?';
      }

      float advance = font.glyphs[c].advance;
      if (x + advance >= textarea_w) {
        x = advance;
        current_line_height += font.line_height;
        continue;
      }

      x += advance;
    }
  } else if (offset_px < 0) {
    /* skip newline and first char (?) */
    iter--;
    iter--;
    while (iter != buffer.begin()) {
      uint8_t c = *iter;
      iter--;

      if (c == '\n' || iter == buffer.begin()) {
        if (offset_px < 0) {
          first_line--;
          offset_px += current_line_height;
        } else {
          break;
        }
        x = 0;
        current_line_height = font.line_height;
        continue;
      }

      if (!isprint(c)) {
        c = '?';
      }

      float advance = font.glyphs[c].advance;
      if (x + advance >= textarea_w) {
        x = advance;
        current_line_height += font.line_height;
        continue;
      }

      x += advance;
    }
  }
}

void ViewEditor::UpdateLayout(void) {
  Hash inputs = Hasher()
                    .add(first_line)
                    /* TODO: handle update time */
                    .add(offset_px)
                    .add(buffer.epoch)
                    .add(viewport.w)
                    .add(viewport.h);
  if (inputs == layout_inputs)
    return;
  layout_inputs = inputs;

  NormalizeCursor();
  layout.clear();
  layout_version++;

  const int textarea_w = viewport.w - CalculateGutterWidth();

  /* cache line length based on hash of line? probably not worth it*/
  /* TODO: normalize offset px */
  int y = -offset_px;
  TextBuffer::iterator iter = buffer.AtLineCol(first_line, 0);
  float x = 0;
  uint16_t run_bytes = 0;
  uint16_t current_line_height = 0;
  bool logical_line_next = true;

  while (y < viewport.h && !iter.IsEOF()) {
    uint8_t c = *iter;
    iter++;

    if (c == '\n') {
      y += font.line_height;
      current_line_height += font.line_height;
      layout.push_back({logical_line_next, true, run_bytes});
      x = 0, run_bytes = 0;
      current_line_height = 0;
      logical_line_next = true;
      continue;
    }

    if (!isprint(c)) {
      c = '?';
    }

    float advance = font.glyphs[c].advance;
    if (x + advance >= textarea_w) {
      layout.push_back({logical_line_next, false, run_bytes});
      x = advance, run_bytes = 1;
      y += font.line_height;
      current_line_height += font.line_height;
      logical_line_next = false;
      continue;
    }

    x += advance;
    run_bytes++;
  }
}

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

  /* TODO: caching */
  is_animating = target_px != 0;

  UpdateLayout();

  Hash inputs = Hasher()
                    .add(layout_version)
                    /* TODO: handle update time */
                    .add(offset_px)
                    .add(first_line)
                    .add(buffer.epoch)
                    .add(viewport.w)
                    .add(viewport.h)
                    .add(viewport.x)
                    .add(viewport.y);

  if (inputs == render_inputs)
    return;

  inputs = render_inputs;

  const int digit_width = font.glyphs['0'].advance;
  const int gutter_width = CalculateGutterWidth();
  render.DrawRect(LayerBg, viewport, RGB(0xf7f4ef));
  render.DrawRect(LayerGutter,
                  {viewport.x, viewport.y, gutter_width, viewport.h}, Dim(0.1));

  int y = viewport.y - offset_px;
  size_t line_num = first_line;
  TextBuffer::iterator iter = buffer.AtLineCol(first_line, 0);
  std::string run;

  for (VisualLine line : layout) {
    if (line.starts_line) {
      drawRun(viewport.x + digit_width, y, std::to_string(line_num + 1));
      line_num++;
    }

    float x = 0;
    run.clear();
    for (uint16_t i = 0; i < line.len_bytes + line.ends_line; i++) {
      uint8_t c = *iter;
      if (iter == *cursor) {
        render.DrawRect(LayerCursor,
                        {viewport.x + gutter_width + (int)x, y + 4, 2,
                         (int)font.line_height},
                        RGB(0x555555));
      }
      iter++;

      if (c == '\n')
        continue;
      if (!isprint(c))
        c = '?';

      run.push_back(c);
      x += font.glyphs[c].advance;
    }
    /* skip newline TODO: handle cursor at end of line */

    drawRun(viewport.x + gutter_width, y, run);
    y += font.line_height;
  }

  /* TODO: use transaction grouping to minimize per edit
   * calculations where we dont care about the intermediate steps */
}

void ViewEditor::drawRun(int x, int y, const std::string &run) {
  if (y + (int)font.line_height < viewport.y)
    return;
  float pos = x;
  for (size_t i = 0; i < run.size(); i++) {
    if (isprint(run[i])) {
      font.DrawGlyph(LayerText, {(int)pos, y + (int)font.line_height}, run[i],
                     RGB(0x111111));
      pos += font.glyphs[run[i]].advance;
    }
  }
}