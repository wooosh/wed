#pragma once

#include "../Render/RenderContext.hxx"
/* TODO: move types in Render/Types.hxx */
#include "Render.hxx"

// only the parent of a view can increase the size of it's region,
// but a view can shrink it's own region
/* TODO: sizepolicy */

struct View {
  /* uses global coords */
  Render::Rect viewport;
  /* TODO: should this be passed as a reference? why not just global editor/draw
   * state */
  virtual void draw(RenderContext &) = 0;
  /* TODO: handle_event */
  virtual ~View() {}
};

/*
ViewScrollable
ViewSplit
*/

class ViewRoot : public View {
  View &child;

public:
  ViewRoot(View &child_view) : child(child_view) {
    viewport.x = 0;
    viewport.y = 0;
  };

  virtual void draw(RenderContext &render) {
    viewport.w = render.win_w;
    viewport.h = render.win_h;
    child.viewport = viewport;
    // render.commit();
    child.draw(render);
  }
};