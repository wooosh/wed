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

  struct VisualLine {
    bool starts_line;
    bool ends_line;
    uint16_t len_bytes;
  };

  /* array of visual lines */
  std::vector<VisualLine> layout;
  uint64_t layout_version;
  Hash layout_inputs;

  /* scroll animation */
  double target_px;
  double progress_target;

  Hash render_inputs;

  std::shared_ptr<TextBuffer::iterator> cursor;

  ViewEditor(RenderFont &font, TextBuffer &buffer)
      : font(font), buffer(buffer), first_line(0), offset_px(0),
        layout_version(0), target_px(0), progress_target(0) {
    is_animating = true;
  };

  void ScrollPx(int amount);
  void ScrollLines(int amount);
  void PageUp(void);
  void PageDown(void);

  int CalculateGutterWidth(void);
  void NormalizeCursor(void);
  void UpdateLayout(void);
  virtual void draw(RenderContext &render);
  void drawRun(int x, int y, const std::string &);
};