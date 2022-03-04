#pragma once

#include "../Render/RenderFont.hxx"
#include "../TextBuffer/TextBuffer.hxx"
#include "View.hxx"

struct ViewEditor : View {
  RenderFont &font;
  TextBuffer &buffer;
  size_t first_line;

  double draw_time;

  ViewEditor(RenderFont &font, TextBuffer &buffer)
      : font(font), buffer(buffer){};

  virtual void draw(RenderContext &render);
  void drawRun(RenderContext &render, uint x, uint y, const std::string &);
};