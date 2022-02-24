#pragma once

#include "../TextBuffer/TextBuffer.hxx"
#include "View.hxx"

struct ViewEditor : View {
  Render::Font &font;
  TextBuffer &buffer;
  size_t first_line;

  double draw_time;

  ViewEditor(Render::Font &font, TextBuffer &buffer)
      : font(font), buffer(buffer){};

  virtual void draw(Render::Backend &render);
  void drawRun(Render::Backend &render, uint x, uint y, std::string);
};