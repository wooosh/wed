#pragma once

#include "Types.hxx"
#include <cstdint>
#include <vector>

struct TileMap {
#define TILEMAP_ROW_BITS 32
  using Row = uint32_t;

  /* both bitmaps must be the same size */
  /* set bit == unused space */
  std::vector<Row> bitmap;
  /* set bit == undamaged space */
  std::vector<Row> bitmap_damaged;

  uint8_t epoch;

  uint8_t tile_w;
  uint8_t tile_h;

  TileMap(uint tile_w, uint tile_h, uint num_rows);
  void Clear();
  Point AllocateRect(uint w, uint h);
  bool IsRectValid(uint8_t epoch, Rect);
};