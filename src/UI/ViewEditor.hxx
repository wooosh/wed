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
  double target_px;
  double progress_target;

  ViewEditor(RenderFont &font, TextBuffer &buffer)
      : font(font), buffer(buffer), first_line(0), offset_px(0), target_px(0),
        progress_target(0){};

  void ScrollPx(int amount);
  void ScrollLines(int amount);
  void PageUp(void);
  void PageDown(void);

  virtual void draw(RenderContext &render);
  void drawRun(RenderContext &render, int x, int y, const std::string &);
};