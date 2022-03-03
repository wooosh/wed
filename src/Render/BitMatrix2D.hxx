#pragma once

#include "Types.hxx"
#include <cstdint>
#include <optional>
#include <vector>

/* TODO: explains how this bounds the size, doesn't change argument order */
/* axis size BitMatrixFixedDimension */
enum class BitMatrix2DFixedAxis { kWidth, kHeight };

void printBits(size_t const size, void const *const ptr) {
  unsigned char *b = (unsigned char *)ptr;
  unsigned char byte;
  int i, j;

  for (i = size - 1; i >= 0; i--) {
    for (j = 7; j >= 0; j--) {
      byte = (b[i] >> j) & 1;
      printf("%u", byte);
    }
  }
  puts("");
}

template <BitMatrix2DFixedAxis order, typename FixedAxis> struct BitMatrix2D {
  constexpr static uint8_t major_len = std::numeric_limits<FixedAxis>::digits;
  using size_type = size_t;

  std::vector<FixedAxis> rows;

  size_type width() {
    if constexpr (order == BitMatrix2DFixedAxis::kWidth) {
      return major_len;
    } else {
      return rows.size();
    }
  }

  size_type height() {
    if constexpr (order == BitMatrix2DFixedAxis::kHeight) {
      return major_len;
    } else {
      return rows.size();
    }
  }

  void Fill(bool value) {
    const FixedAxis v = value && ~(FixedAxis)0;
    for (FixedAxis &x : rows) {
      x = v;
    }
  }

  void SetRect(Rect r) {
    if constexpr (order == BitMatrix2DFixedAxis::kHeight) {
      std::swap(r.x, r.y);
      std::swap(r.w, r.h);
    }

    FixedAxis mask = ((FixedAxis)(~(FixedAxis)0) << (major_len - r.w)) >> r.x;
    for (size_type y = r.y; y < r.y + r.h; y++) {
      rows[y] |= mask;
    }
  };

  std::optional<vec2<size_type>> FindUnsetRect(size_type w, size_type h) {
    uint_fast8_t min_bit_width;
    size_type min_run_len;

    if constexpr (order == BitMatrix2DFixedAxis::kWidth) {
      min_bit_width = w;
      min_run_len = h;
    } else {
      min_run_len = w;
      min_bit_width = h;
    }

    FixedAxis *row = rows.data();
    const FixedAxis *end = row + rows.size();

    uint32_t minor_idx = 0;
    size_type run_len = 0;
    constexpr static uint32_t q = major_len - 1;
    while (row < end && run_len <= min_run_len) {
      uint32_t bits_unset =
          __builtin_clz((*row << minor_idx) | (FixedAxis)1 << minor_idx);
      uint32_t bits_set = __builtin_clz(~(*row << minor_idx));

      bool is_match = bits_unset >= min_bit_width;

      minor_idx += !is_match * (bits_unset + bits_set);

      row += (minor_idx >= q);
      row -= !is_match * run_len;
      row += is_match;

      run_len = is_match * (run_len + 1);
      minor_idx *= (q > minor_idx);
    }

    // for (size_t i = 0; i < rows.size(); i++) {
    //  printf("%o\n", rows[i]);
    //}

    /* TODO: will exit early, fix */
    if (row == end) {
      return std::nullopt;
    }

    size_type i = (row - rows.data()) - run_len;
    if constexpr (order == BitMatrix2DFixedAxis::kWidth) {
      return std::optional<vec2<size_type>>({minor_idx, i});
    } else {
      return std::optional<vec2<size_type>>({i, minor_idx});
    }
  }
};
