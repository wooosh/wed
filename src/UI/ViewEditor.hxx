#pragma once

#include "../Render/RenderFont.hxx"
#include "../TextBuffer/TextBuffer.hxx"
#include "View.hxx"

struct ViewEditor : View {
  RenderFont &font;
  TextBuffer &buffer;
  /* current scroll info */
  size_t first_line;
  int64_t offset_px;

  /* scroll animation */
  int64_t remaining_delta;

  ViewEditor(RenderFont &font, TextBuffer &buffer)
      : font(font), buffer(buffer), first_line(0), offset_px(0),
        remaining_delta(0){};

  void ScrollPx(int amount);
  virtual void draw(RenderContext &render);
  void drawRun(RenderContext &render, int x, int y, const std::string &);
};