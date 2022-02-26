#pragma once

#include <cstdint>
namespace Render {

enum CommandType {
  COMMAND_RECT = 0b10,
  COMMAND_TEXTURE = 0b00,
  COMMAND_TEXT = 0b10
};

struct BatchKey {
  uint8_t command_type;
  uint8_t glyph_atlas_idx;
};

struct Batch {
  uint8_t command_type;
};

struct RenderContext {
  /* how does batching interact with transparency */
  /* used to control Z depth, can be incremented/decremented by the user */
  int layer;

  void DrawRectColor();
  void DrawRectTexture();
  void DrawRectMasked();

  void DrawText();
};
} // namespace Render