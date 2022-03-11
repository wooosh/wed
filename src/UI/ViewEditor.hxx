#pragma once

#include "../Render/RenderFont.hxx"
#include "../TextBuffer/TextBuffer.hxx"
#include "../Util/Hash.hxx"
#include "View.hxx"
#include <memory>

struct ViewEditor : View {
  RenderFont &font;
  TextBuffer &buffer;
  /* current scroll info */
  size_t first_line; /* TODO: handle being out of range, maybe use an iterator
                        for it */
  int64_t offset_px;

  /* scroll animation */
  double target_px;
  double progress_target;

  Hash last_inputs;

  std::shared_ptr<TextBuffer::iterator> cursor;

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