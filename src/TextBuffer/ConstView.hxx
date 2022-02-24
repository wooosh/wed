#pragma once
#include <iterator>

template <typename T> struct ConstView {
  const T *begin;
  size_t len;

  auto cbegin() const { return begin; }
  auto cend() const { return begin + len; }
  auto crbegin() const { return std::make_reverse_iterator(begin); }
  auto crend() const { return std::make_reverse_iterator(begin + len); }

  const T &operator[](size_t idx) { return begin[idx]; }
};
