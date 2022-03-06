#pragma once

#include "../Render/RenderFont.hxx"
#include "../TextBuffer/TextBuffer.hxx"
#include "View.hxx"

struct ViewEditor : View {
  RenderFont &font;
  TextBuffer &buffer;
  size_t first_line;
  int64_t offset_px;

  double draw_time;

  ViewEditor(RenderFont &font, TextBuffer &buffer)
      : font(font), buffer(buffer){};

  void ScrollPx(int amount);
  virtual void draw(RenderContext &render);
  void drawRun(RenderContext &render, int x, int y, const std::string &);
};