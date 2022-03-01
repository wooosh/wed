#include "TileMap.hxx"
#include "../Util/Assert.hxx"
#include <cassert>
#include <cstdint>
#include <cstdio>

TileMap::TileMap(uint tile_w, uint tile_h, uint num_rows) {
  assume(num_rows < UINT8_MAX && tile_w < UINT8_MAX && tile_h < UINT8_MAX,
         "exceeds uint8");
  this->tile_w = tile_w;
  this->tile_h = tile_h;
  this->bitmap = std::vector<Row>(num_rows);
  this->bitmap_damaged = std::vector<Row>(num_rows);
  Clear();
}

void TileMap::Clear() {
  for (uint8_t i = 0; i < bitmap.size(); i++) {
    bitmap[i] = ~(Row)0;
    bitmap_damaged[i] = ~(Row)0;
  }
}

Point TileMap::AllocateRect(uint w_px, uint h_px) {
  /* convert px to tiles, rounding upward */
  const uint8_t w_tiles = (w_px + tile_w - 1) / tile_w;
  const uint8_t h_tiles = (h_px + tile_h - 1) / tile_h;

  /* TODO: move this check elsewhere */
  assume(w_tiles < 32 && h_tiles < bitmap.size(), "too large for bitmap");

  const Row row_mask = ~(Row)0 << (TILEMAP_ROW_BITS - w_tiles);

  Row *row = bitmap.data();
  Row *end = bitmap.data() + bitmap.size();

  uint8_t col_idx = 0;
  uint8_t rows_matched = 0;

  while (row < end && rows_matched < h_tiles) {
    bool row_match = row_mask == (row_mask & ((*row) << col_idx));

    if (row_match) {
      rows_matched++;
      row++;
    } else {
      col_idx++;
      if (col_idx >= TILEMAP_ROW_BITS) {
        row++;
      }
      col_idx %= TILEMAP_ROW_BITS;
      /* could use last_state */
      if (rows_matched != 0) {
        row -= rows_matched;
        rows_matched = 0;
      }
    }
  }

  /* set bits */
  const uint8_t y_tiles = (row - bitmap.data()) - h_tiles;
  const Row row_write_mask = ~(row_mask >> col_idx);
  for (uint8_t y = 0; y < h_tiles; y++) {
    bitmap[y_tiles + y] &= row_write_mask;
    bitmap_damaged[y_tiles + y] &= row_write_mask;
  }

  /* TODO: if (row == end) */
  return {(uint)(col_idx * tile_w), (uint)y_tiles * tile_h};
}

bool TileMap::IsRectValid(uint8_t r_epoch, Rect r) {
  if (r_epoch == epoch)
    return true;

  /* coordinates must be at a specific tile */
  assert(r.x % tile_w == 0 && r.y % tile_h);

  const uint8_t y_tiles = r.y / tile_w;
  const uint8_t w_tiles = (r.w + tile_w - 1) / tile_w;
  const uint8_t h_tiles = (r.h + tile_h - 1) / tile_h;
  const Row row_mask = ~(Row)0 << (TILEMAP_ROW_BITS - w_tiles);
  for (uint8_t y = 0; y < h_tiles; y++) {
    if (row_mask != (bitmap_damaged[y_tiles + y] & row_mask)) {
      return false;
    }
  }

  return true;
}