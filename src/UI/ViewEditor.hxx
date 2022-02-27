#pragma once

#include "../TextBuffer/TextBuffer.hxx"
#include "View.hxx"

struct ViewEditor : View {
  GlyphAtlas &font;
  TextBuffer &buffer;
  size_t first_line;

  double draw_time;

  ViewEditor(GlyphAtlas &font, TextBuffer &buffer)
      : font(font), buffer(buffer){};

  virtual void draw(RenderContext &render);
  void drawRun(RenderContext &render, uint x, uint y, const std::string &);
};